/*
 *  Copyright (c) 2008-2012 Mike Massonnet <mmassonnet@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "collector.h"
#include "common.h"
#include "history.h"
#include "menu.h"

#include <libxfce4ui/libxfce4ui.h>

#ifdef HAVE_QRENCODE
#include <qrencode.h>
#endif

/*
 * GObject declarations
 */

struct _ClipmanMenuPrivate
{
  GtkWidget *mi_clear_history;
  ClipmanHistory *history;
  GSList *list;
  gboolean reverse_order;
#ifdef HAVE_QRENCODE
  gboolean show_qr_code;
#endif
  guint paste_on_activate;
  guint max_menu_items;
  gboolean never_confirm_history_clear;
};

G_DEFINE_TYPE_WITH_PRIVATE (ClipmanMenu, clipman_menu, GTK_TYPE_MENU)

enum
{
  REVERSE_ORDER = 1,
#ifdef HAVE_QRENCODE
  SHOW_QR_CODE,
#endif
  PASTE_ON_ACTIVATE,
  NEVER_CONFIRM_HISTORY_CLEAR,
  MAX_MENU_ITEMS,
};

static void
clipman_menu_finalize (GObject *object);
static void
clipman_menu_set_property (GObject *object,
                           guint property_id,
                           const GValue *value,
                           GParamSpec *pspec);
static void
clipman_menu_get_property (GObject *object,
                           guint property_id,
                           GValue *value,
                           GParamSpec *pspec);
#ifdef HAVE_QRENCODE
GdkPixbuf *
clipman_menu_qrcode (char *text,
                     gint scale_factor);
#endif


/*
 * Callbacks declarations
 */

#ifdef HAVE_QRENCODE
static void
cb_set_qrcode (GtkMenuItem *mi,
               const GdkPixbuf *pixbuf);
#endif
static void
cb_set_clipboard (GtkMenuItem *mi,
                  const ClipmanHistoryItem *item);
static void
cb_clear_history (ClipmanMenu *menu);



/*
 * Callbacks
 */

#ifdef HAVE_QRENCODE
static void
cb_set_qrcode (GtkMenuItem *mi,
               const GdkPixbuf *pixbuf)
{
  GtkClipboard *clipboard;
  ClipmanCollector *collector;
  ClipmanHistory *history;

  history = clipman_history_get ();
  clipman_history_add_image (history, pixbuf);
  g_object_unref (history);

  collector = clipman_collector_get ();
  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  clipman_collector_set_is_restoring (collector, clipboard);
  g_object_unref (collector);

  gtk_clipboard_set_image (clipboard, GDK_PIXBUF (pixbuf));
}
#endif

static void
cb_set_clipboard_from_primary (GtkMenuItem *mi,
                               GObject *menu)
{
  GtkClipboard *clipboard;

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (clipboard, g_object_get_data (menu, "selection-primary"), -1);
}

static void
cb_set_clipboard (GtkMenuItem *mi,
                  const ClipmanHistoryItem *item)
{
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  switch (item->type)
    {
    case CLIPMAN_HISTORY_TYPE_TEXT:
      DBG ("Copy text `%s' to default clipboard", item->content.text);
      gtk_clipboard_set_text (clipboard, item->content.text, -1);
      break;

    case CLIPMAN_HISTORY_TYPE_IMAGE:
      DBG ("Copy image (%p) to default clipboard", item->content.image);
      gtk_clipboard_set_image (clipboard, item->content.image);
      break;

    default:
      DBG ("Ignoring unknown history type %d", item->type);
      return;
    }

  clipman_common_paste_on_activate (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (mi), "paste-on-activate")));
}

static void
cb_clear_history (ClipmanMenu *menu)
{
  gint res;
  GtkWidget *dialog;
  GtkClipboard *clipboard;
  ClipmanCollector *collector;

  if (!menu->priv->never_confirm_history_clear)
    {
      dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                       _("Are you sure you want to clear the history?"));


      {
        GtkWidget *content_area = gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (dialog));
        GtkWidget *checkbox = gtk_check_button_new_with_label (_("Don't ask again"));
        g_object_bind_property (G_OBJECT (checkbox), "active",
                                G_OBJECT (menu), "never-confirm-history-clear",
                                G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
        gtk_widget_show (checkbox);
        gtk_container_add (GTK_CONTAINER (content_area), checkbox);

        res = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        if (res != GTK_RESPONSE_YES)
          {
            g_object_set (menu, "never-confirm-history-clear", FALSE, NULL);
            return;
          }
      }
    }

  clipman_history_clear (menu->priv->history);

  /* prevent persistent-primary-clipboard from restoring the selection and do not
   * restore default clipboard either */
  collector = clipman_collector_get ();
  clipman_collector_clear_cache (collector);
  g_object_unref (collector);

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (clipboard, "", -1);
  gtk_clipboard_clear (clipboard);

  clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  gtk_clipboard_set_text (clipboard, "", -1);
  gtk_clipboard_clear (clipboard);
}

static void
cb_launch_clipman_bin (ClipmanMenu *menu,
                       gpointer user_data)
{
  GError *error = NULL;
  GtkWidget *error_dialog;
  gchar *command = user_data;

  g_spawn_command_line_async (command, &error);

  if (error != NULL)
    {
      error_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                             _("Unable to open the Clipman history dialog"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog), "%s", error->message);
      gtk_dialog_run (GTK_DIALOG (error_dialog));
      gtk_widget_destroy (error_dialog);
      g_error_free (error);
    }
}

/*
 * Private methods
 */

static void
_clipman_menu_adjust_geometry (ClipmanMenu *menu)
{
  GtkAllocation allocation = {};

  gtk_widget_get_preferred_width (GTK_WIDGET (menu), NULL, &allocation.width);
  gtk_widget_get_preferred_height (GTK_WIDGET (menu), NULL, &allocation.height);
  gtk_widget_size_allocate (GTK_WIDGET (menu), &allocation);
}

static void
_clipman_menu_update_list (ClipmanMenu *menu)
{
  GtkWidget *mi, *image;
  GdkPixbuf *pixbuf;
  ClipmanCollector *collector;
  ClipmanHistoryItem *item;
  const ClipmanHistoryItem *item_to_restore = NULL;
  GSList *list, *l;
  gint pos = 0;
  guint i = 0, offset = 0;
  const gchar *selection_primary;
  const gchar *selection_clipboard;
  GBytes *pixel_bytes = NULL;
  gchar *selection_primary_short;
  gboolean skip_primary = FALSE;
  cairo_surface_t *surface;
  gint scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (menu));

  /* retrieve clipboard and primary selections */
  selection_clipboard = g_object_get_data (G_OBJECT (menu), "selection-clipboard");
  collector = clipman_collector_get ();
  if ((pixbuf = clipman_collector_get_current_image (collector)) != NULL)
    pixel_bytes = gdk_pixbuf_read_pixel_bytes (pixbuf);
  g_object_unref (collector);

  selection_primary = g_object_get_data (G_OBJECT (menu), "selection-primary");
  if (selection_primary == NULL)
    skip_primary = TRUE;

  /* Clear the previous menu items */
  g_slist_free_full (menu->priv->list, (GDestroyNotify) gtk_widget_destroy);
  menu->priv->list = NULL;

  /* Set the clear history item sensitive */
  gtk_widget_set_sensitive (menu->priv->mi_clear_history, TRUE);
  gtk_menu_item_set_label (GTK_MENU_ITEM (menu->priv->mi_clear_history), _("_Clear history"));

  /* Insert an updated list of menu items */
  list = clipman_history_get_list (menu->priv->history);
  if (menu->priv->reverse_order)
    {
      list = g_slist_reverse (list);
      offset = MAX (0, (gint) (g_slist_length (list) - menu->priv->max_menu_items));
    }

  for (i = 0, l = g_slist_nth (list, offset); i < menu->priv->max_menu_items; i++, l = l->next)
    {
      if (l == NULL)
        break;
      item = l->data;

      switch (item->type)
        {
        case CLIPMAN_HISTORY_TYPE_TEXT:
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          mi = gtk_image_menu_item_new_with_label (item->preview.text);
          G_GNUC_END_IGNORE_DEPRECATIONS
          if (item_to_restore == NULL && g_strcmp0 (selection_clipboard, item->content.text) == 0)
            {
              item_to_restore = item;
              image = gtk_image_new_from_icon_name ("edit-paste-symbolic", GTK_ICON_SIZE_MENU);
              G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
              G_GNUC_END_IGNORE_DEPRECATIONS
              if (g_strcmp0 (selection_primary, item->content.text) == 0)
                skip_primary = TRUE;
            }
          else if (!skip_primary && g_strcmp0 (selection_primary, item->content.text) == 0)
            {
              skip_primary = TRUE;
              image = gtk_image_new_from_icon_name ("input-mouse-symbolic", GTK_ICON_SIZE_MENU);
              G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
              G_GNUC_END_IGNORE_DEPRECATIONS
            }
          break;

        case CLIPMAN_HISTORY_TYPE_IMAGE:
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          mi = gtk_image_menu_item_new ();
          G_GNUC_END_IGNORE_DEPRECATIONS
          surface = gdk_cairo_surface_create_from_pixbuf (item->preview.image, scale_factor, NULL);
          image = gtk_image_new_from_surface (surface);
          cairo_surface_destroy (surface);
          gtk_container_add (GTK_CONTAINER (mi), image);
          if (item_to_restore == NULL && pixel_bytes != NULL && g_bytes_equal (pixel_bytes, item->pixel_bytes))
            {
              item_to_restore = item;
              image = gtk_image_new_from_icon_name ("edit-paste-symbolic", GTK_ICON_SIZE_MENU);
              G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
              G_GNUC_END_IGNORE_DEPRECATIONS
            }
          break;

        default:
          DBG ("Ignoring unknown history type %d", item->type);
          continue;
        }

      g_signal_connect (mi, "activate", G_CALLBACK (cb_set_clipboard), item);
      g_object_set_data (G_OBJECT (mi), "paste-on-activate", GUINT_TO_POINTER (menu->priv->paste_on_activate));
      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
      gtk_widget_show_all (mi);
    }

  g_slist_free (list);
  if (pixel_bytes != NULL)
    g_bytes_unref (pixel_bytes);

  if (pos == 0)
    {
      /* Insert empty menu item */
      mi = gtk_menu_item_new_with_label (_("History is empty"));
      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, 0);
      gtk_widget_set_sensitive (mi, FALSE);
      gtk_widget_show (mi);

      /* Set the clear history item insensitive */
      gtk_widget_set_sensitive (menu->priv->mi_clear_history, FALSE);
    }

  /* Show the primary clipboard item so it can be selected for keyboard pasting
   * even if history-ignore-primary-clipboard is enabled: the separator below should
   * make it clear that it is not a history entry */
  if (!skip_primary)
    {
      gboolean reverse_order = menu->priv->reverse_order;
      if (!gtk_widget_get_sensitive (menu->priv->mi_clear_history))
        {
          reverse_order = FALSE;

          /* Set the clear history item sensitive */
          gtk_widget_set_sensitive (menu->priv->mi_clear_history, TRUE);
          gtk_menu_item_set_label (GTK_MENU_ITEM (menu->priv->mi_clear_history), _("_Clear clipboard"));
        }

      mi = gtk_separator_menu_item_new ();
      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, reverse_order ? pos++ : 0);
      gtk_widget_show_all (mi);

      selection_primary_short = clipman_common_shorten_preview (selection_primary);
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      mi = gtk_image_menu_item_new_with_label (selection_primary_short);
      image = gtk_image_new_from_icon_name ("input-mouse-symbolic", GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
      G_GNUC_END_IGNORE_DEPRECATIONS
      g_free (selection_primary_short);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, reverse_order ? pos++ : 0);
      gtk_widget_show_all (mi);
      g_signal_connect (mi, "activate", G_CALLBACK (cb_set_clipboard_from_primary), menu);
      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
    }

#ifdef HAVE_QRENCODE
  /* Draw QR Code if clipboard content is text */
  if (menu->priv->show_qr_code && item_to_restore && item_to_restore->type == CLIPMAN_HISTORY_TYPE_TEXT)
    {
      mi = gtk_separator_menu_item_new ();
      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
      gtk_widget_show_all (mi);

      if ((pixbuf = clipman_menu_qrcode (item_to_restore->content.text, scale_factor)) != NULL)
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          mi = gtk_image_menu_item_new ();
          G_GNUC_END_IGNORE_DEPRECATIONS
          surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);
          gtk_container_add (GTK_CONTAINER (mi), gtk_image_new_from_surface (surface));
          cairo_surface_destroy (surface);
          g_object_set_data_full (G_OBJECT (mi), "pixbuf", pixbuf, g_object_unref);
          g_signal_connect (mi, "activate", G_CALLBACK (cb_set_qrcode), pixbuf);
          menu->priv->list = g_slist_prepend (menu->priv->list, mi);
          gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
          gtk_widget_show_all (mi);
        }
      else
        {
          mi = gtk_menu_item_new_with_label (_("Could not generate QR-Code."));
          menu->priv->list = g_slist_prepend (menu->priv->list, mi);
          gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
          gtk_widget_set_sensitive (mi, FALSE);
          gtk_widget_show (mi);
        }
    }
#endif

  _clipman_menu_adjust_geometry (menu);
}

/*
 * Public methods
 */

ClipmanMenu *
clipman_menu_new (void)
{
  return g_object_ref_sink (g_object_new (CLIPMAN_TYPE_MENU, NULL));
}

/*
 * GObject
 */

static void
clipman_menu_class_init (ClipmanMenuClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = clipman_menu_finalize;
  object_class->set_property = clipman_menu_set_property;
  object_class->get_property = clipman_menu_get_property;

  g_object_class_install_property (object_class, REVERSE_ORDER,
                                   g_param_spec_boolean ("reverse-order",
                                                         "ReverseOrder",
                                                         "Set to TRUE to display the menu in the reverse order",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

#ifdef HAVE_QRENCODE
  g_object_class_install_property (object_class, SHOW_QR_CODE,
                                   g_param_spec_boolean ("show-qr-code",
                                                         "ShowQrCode",
                                                         "Set to TRUE to display QR-Code in the menu",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
#endif

  g_object_class_install_property (object_class, PASTE_ON_ACTIVATE,
                                   g_param_spec_uint ("paste-on-activate",
                                                      "PasteOnActivate",
                                                      "Paste the content of a menu item when it is activated",
                                                      0, 2, 0,
                                                      G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, NEVER_CONFIRM_HISTORY_CLEAR,
                                   g_param_spec_boolean ("never-confirm-history-clear",
                                                         "NeverConfirmHistoryClear",
                                                         "Set to FALSE to clear the history list with confirmation",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, MAX_MENU_ITEMS,
                                   g_param_spec_uint ("max-menu-items",
                                                      "MaxMenuItems",
                                                      "Maximum amount of items displayed in the plugin's menu",
                                                      1, 100, 15,
                                                      G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
clipman_menu_init (ClipmanMenu *menu)
{
  GtkWidget *mi;
  GtkWidget *image;
  guint max_texts_in_history;

  menu->priv = clipman_menu_get_instance_private (menu);

  /* ClipmanHistory */
  menu->priv->history = clipman_history_get ();
  clipman_history_set_scale_factor (menu->priv->history, NULL, GTK_WIDGET (menu));
  g_signal_connect_object (menu, "notify::scale-factor",
                           G_CALLBACK (clipman_history_set_scale_factor),
                           menu->priv->history, G_CONNECT_SWAPPED);

  /* Connect signal on show to update the items */
  g_signal_connect_swapped (menu, "show", G_CALLBACK (_clipman_menu_update_list), menu);

  /* Footer items */
  mi = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

  max_texts_in_history = clipman_history_get_max_texts_in_history (menu->priv->history);
  if (max_texts_in_history > menu->priv->max_menu_items)
    {
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      mi = gtk_image_menu_item_new_with_mnemonic (_("_Show full history..."));
      image = gtk_image_new_from_icon_name ("accessories-dictionary-symbolic", GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
      G_GNUC_END_IGNORE_DEPRECATIONS
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect (mi, "activate", G_CALLBACK (cb_launch_clipman_bin), "xfce4-clipman-history");
    }

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  menu->priv->mi_clear_history = mi = gtk_image_menu_item_new_with_mnemonic (_("_Clear history"));
  image = gtk_image_new_from_icon_name ("edit-clear-symbolic", GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu->priv->mi_clear_history), image);
  G_GNUC_END_IGNORE_DEPRECATIONS
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (mi, "activate", G_CALLBACK (cb_clear_history), menu);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  mi = gtk_image_menu_item_new_with_mnemonic (_("_Clipman settings..."));
  image = gtk_image_new_from_icon_name ("preferences-system-symbolic", GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
  G_GNUC_END_IGNORE_DEPRECATIONS
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect (mi, "activate", G_CALLBACK (cb_launch_clipman_bin), "xfce4-clipman-settings");

  /* Show all the items */
  gtk_widget_show_all (GTK_WIDGET (menu));
}

static void
clipman_menu_finalize (GObject *object)
{
  ClipmanMenu *menu = CLIPMAN_MENU (object);

  g_object_unref (menu->priv->history);

  G_OBJECT_CLASS (clipman_menu_parent_class)->finalize (object);
}

static void
clipman_menu_set_property (GObject *object,
                           guint property_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  ClipmanMenuPrivate *priv = CLIPMAN_MENU (object)->priv;

  switch (property_id)
    {
    case REVERSE_ORDER:
      priv->reverse_order = g_value_get_boolean (value);
      break;

#ifdef HAVE_QRENCODE
    case SHOW_QR_CODE:
      priv->show_qr_code = g_value_get_boolean (value);
      break;
#endif

    case PASTE_ON_ACTIVATE:
      priv->paste_on_activate = g_value_get_uint (value);
      break;

    case NEVER_CONFIRM_HISTORY_CLEAR:
      priv->never_confirm_history_clear = g_value_get_boolean (value);
      break;

    case MAX_MENU_ITEMS:
      priv->max_menu_items = g_value_get_uint (value);
      break;

    default:
      break;
    }
}

static void
clipman_menu_get_property (GObject *object,
                           guint property_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  ClipmanMenuPrivate *priv = CLIPMAN_MENU (object)->priv;

  switch (property_id)
    {
    case REVERSE_ORDER:
      g_value_set_boolean (value, priv->reverse_order);
      break;

    case PASTE_ON_ACTIVATE:
      g_value_set_uint (value, priv->paste_on_activate);
      break;

    case NEVER_CONFIRM_HISTORY_CLEAR:
      g_value_set_boolean (value, priv->never_confirm_history_clear);
      break;

    case MAX_MENU_ITEMS:
      g_value_set_uint (value, priv->max_menu_items);
      break;

    default:
      break;
    }
}

#ifdef HAVE_QRENCODE
GdkPixbuf *
clipman_menu_qrcode (char *text,
                     gint scale_factor)
{
  QRcode *qrcode;
  GdkPixbuf *pixbuf, *pixbuf_scaled;
  int i, j, k, rowstride, channels, size;
  guchar *pixel;
  unsigned char *data;

  qrcode = QRcode_encodeString8bit (text, 0, QR_ECLEVEL_L);

  if (qrcode == NULL)
    return NULL;

  data = qrcode->data;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, qrcode->width + 2, qrcode->width + 2);

  pixel = gdk_pixbuf_get_pixels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  channels = gdk_pixbuf_get_n_channels (pixbuf);

  gdk_pixbuf_fill (pixbuf, 0xffffffff);
  for (i = 1; i <= qrcode->width; i++)
    for (j = 1; j <= qrcode->width; j++)
      {
        for (k = 0; k < channels; k++)
          pixel[i * rowstride + j * channels + k] = !(*data & 0x1) * 0xff;
        data++;
      }

  size = (qrcode->width + 2) * 3 * scale_factor;
  pixbuf_scaled = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_NEAREST);

  QRcode_free (qrcode);
  g_object_unref (pixbuf);

  return pixbuf_scaled;
}
#endif
