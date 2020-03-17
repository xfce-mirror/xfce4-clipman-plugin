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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

#ifdef HAVE_QRENCODE
#include <qrencode.h>
#endif

#include "common.h"
#include "collector.h"
#include "history.h"

#include "menu.h"

/*
 * GObject declarations
 */

G_DEFINE_TYPE (ClipmanMenu, clipman_menu, GTK_TYPE_MENU)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CLIPMAN_TYPE_MENU, ClipmanMenuPrivate))

struct _ClipmanMenuPrivate
{
  GtkWidget            *mi_clear_history;
  ClipmanHistory       *history;
  GSList               *list;
  gboolean              reverse_order;
#ifdef HAVE_QRENCODE
  gboolean              show_qr_code;
#endif
  guint                 paste_on_activate;
  gboolean              never_confirm_history_clear;
};

enum
{
  REVERSE_ORDER = 1,
#ifdef HAVE_QRENCODE
  SHOW_QR_CODE,
#endif
  PASTE_ON_ACTIVATE,
  NEVER_CONFIRM_HISTORY_CLEAR,
};

static void             clipman_menu_finalize           (GObject *object);
static void             clipman_menu_set_property       (GObject *object,
                                                         guint property_id,
                                                         const GValue *value,
                                                         GParamSpec *pspec);
static void             clipman_menu_get_property       (GObject *object,
                                                         guint property_id,
                                                         GValue *value,
                                                         GParamSpec *pspec);
#ifdef HAVE_QRENCODE
GdkPixbuf *             clipman_menu_qrcode             (char *text);
#endif


/*
 * Private methods declarations
 */

static void            _clipman_menu_free_list          (ClipmanMenu *menu);

/*
 * Callbacks declarations
 */

#ifdef HAVE_QRENCODE
static void		cb_set_qrcode                   (GtkMenuItem *mi,
                                                         const GdkPixbuf *pixbuf);
#endif
static void             cb_set_clipboard                (GtkMenuItem *mi,
                                                         const ClipmanHistoryItem *item);
static void             cb_clear_history                (ClipmanMenu *menu);



/*
 * Callbacks
 */

#ifdef HAVE_QRENCODE
static void
cb_set_qrcode (GtkMenuItem *mi, const GdkPixbuf *pixbuf)
{
  GtkClipboard *clipboard;
  ClipmanCollector *collector;
  ClipmanHistory *history;

  collector = clipman_collector_get ();
  clipman_collector_set_is_restoring (collector);
  g_object_unref (collector);

  history = clipman_history_get ();
  clipman_history_add_image (history, pixbuf);

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_image (clipboard, GDK_PIXBUF (pixbuf));

  g_object_unref (history);
}
#endif

static void
cb_set_clipboard (GtkMenuItem *mi, const ClipmanHistoryItem *item)
{
  GtkClipboard *clipboard;
  ClipmanCollector *collector;
  ClipmanHistory *history;

  switch (item->type)
    {
    case CLIPMAN_HISTORY_TYPE_TEXT:
      clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
      gtk_clipboard_set_text (clipboard, item->content.text, -1);

      clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
      gtk_clipboard_set_text (clipboard, item->content.text, -1);
      break;

    case CLIPMAN_HISTORY_TYPE_IMAGE:
      DBG ("Copy image (%p) to default clipboard", item->content.image);

      collector = clipman_collector_get ();
      clipman_collector_set_is_restoring (collector);
      g_object_unref (collector);

      history = clipman_history_get ();
      clipman_history_set_item_to_restore (history, item);
      g_object_unref (history);

      clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
      gtk_clipboard_set_image (clipboard, GDK_PIXBUF (item->content.image));
      break;

    default:
      DBG("Ignoring unknown history type %d", item->type);
      return;
    }

  cb_paste_on_activate (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (mi), "paste-on-activate")));
}

void
cb_paste_on_activate (guint paste_on_activate)
{
  int dummyi;
  KeySym key_sym;
  KeyCode key_code;

  g_warning ("paste on activate...");

  Display *display = XOpenDisplay (NULL);
  if (display == NULL)
    {
      return;
    }
  else if (!XQueryExtension (display, "XTEST", &dummyi, &dummyi, &dummyi))
    {
      XCloseDisplay (display);
      return;
    }
  g_warning ("moving on... %d", paste_on_activate);

  switch (paste_on_activate)
    {
    case PASTE_INACTIVE:
      break;

    case PASTE_CTRL_V:
      g_warning ("ctrl + v");
      key_sym = XK_Control_L;
      key_code = XKeysymToKeycode (display, key_sym);
      XTestFakeKeyEvent (display, key_code, True, CurrentTime);
      key_sym = XK_v;
      key_code = XKeysymToKeycode (display, key_sym);
      XTestFakeKeyEvent (display, key_code, True, CurrentTime);
      key_sym = XK_v;
      key_code = XKeysymToKeycode (display, key_sym);
      XTestFakeKeyEvent (display, key_code, False, CurrentTime);
      key_sym = XK_Control_L;
      key_code = XKeysymToKeycode (display, key_sym);
      XTestFakeKeyEvent (display, key_code, False, CurrentTime);
      break;

    case PASTE_SHIFT_INS:
      key_sym = XK_Shift_L;
      key_code = XKeysymToKeycode (display, key_sym);
      XTestFakeKeyEvent (display, key_code, True, CurrentTime);
      key_sym = XK_Insert;
      key_code = XKeysymToKeycode (display, key_sym);
      XTestFakeKeyEvent (display, key_code, True, CurrentTime);
      key_sym = XK_Insert;
      key_code = XKeysymToKeycode (display, key_sym);
      XTestFakeKeyEvent (display, key_code, False, CurrentTime);
      key_sym = XK_Shift_L;
      key_code = XKeysymToKeycode (display, key_sym);
      XTestFakeKeyEvent (display, key_code, False, CurrentTime);
      break;
    }

  XCloseDisplay (display);
}

static void
cb_clear_history (ClipmanMenu *menu)
{
  gint res;
  GtkWidget *dialog;
  GtkClipboard *clipboard;

  if (!menu->priv->never_confirm_history_clear)
    {
      dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                       _("Are you sure you want to clear the history?"));


      {
        GtkWidget *content_area = gtk_message_dialog_get_message_area (GTK_MESSAGE_DIALOG (dialog));
        GtkWidget *checkbox = gtk_check_button_new_with_label (_("Don't ask again"));
        g_object_bind_property(G_OBJECT (checkbox), "active",
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

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (clipboard, "", 1);
  gtk_clipboard_clear (clipboard);

  clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  gtk_clipboard_set_text (clipboard, "", 1);
  gtk_clipboard_clear (clipboard);
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
#ifdef HAVE_QRENCODE
  GdkPixbuf *pixbuf;
#endif
  ClipmanHistoryItem *item;
  const ClipmanHistoryItem *item_to_restore;
  GSList *list, *l;
  gint pos = 0;

  /* Get the most recent item in the history */
  item_to_restore = clipman_history_get_item_to_restore (menu->priv->history);

  /* Clear the previous menu items */
  _clipman_menu_free_list (menu);

  /* Set the clear history item sensitive */
  gtk_widget_set_sensitive (menu->priv->mi_clear_history, TRUE);

  /* Insert an updated list of menu items */
  list = clipman_history_get_list (menu->priv->history);
  if (menu->priv->reverse_order)
    list = g_slist_reverse (list);
  for (l = list; l != NULL; l = l->next)
    {
      item = l->data;

      switch (item->type)
        {
        case CLIPMAN_HISTORY_TYPE_TEXT:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          mi = gtk_image_menu_item_new_with_label (item->preview.text);
G_GNUC_END_IGNORE_DEPRECATIONS
          break;

        case CLIPMAN_HISTORY_TYPE_IMAGE:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          mi = gtk_image_menu_item_new ();
          image = gtk_image_new_from_pixbuf (item->preview.image);
          gtk_container_add (GTK_CONTAINER (mi), image);
G_GNUC_END_IGNORE_DEPRECATIONS
          break;

        default:
          DBG("Ignoring unknown history type %d", item->type);
          continue;
        }

      g_signal_connect (mi, "activate", G_CALLBACK (cb_set_clipboard), item);
      g_object_set_data (G_OBJECT (mi), "paste-on-activate", GUINT_TO_POINTER (menu->priv->paste_on_activate));

      if (item == item_to_restore)
        {
          image = gtk_image_new_from_icon_name ("go-next-symbolic", GTK_ICON_SIZE_MENU);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
G_GNUC_END_IGNORE_DEPRECATIONS
        }

      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
      gtk_widget_show_all (mi);
    }

#ifdef HAVE_QRENCODE
  /* Draw QR Code if clipboard content is text */
  if (menu->priv->show_qr_code && item_to_restore && item_to_restore->type == CLIPMAN_HISTORY_TYPE_TEXT)
    {
      mi = gtk_separator_menu_item_new ();
      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
      gtk_widget_show_all (mi);

      if ((pixbuf = clipman_menu_qrcode (item_to_restore->content.text)) != NULL)
        {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          mi = gtk_image_menu_item_new ();
G_GNUC_END_IGNORE_DEPRECATIONS
          gtk_container_add (GTK_CONTAINER (mi), gtk_image_new_from_pixbuf (pixbuf));
          g_signal_connect (mi, "activate", G_CALLBACK (cb_set_qrcode), pixbuf);
          menu->priv->list = g_slist_prepend (menu->priv->list, mi);
          gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
          gtk_widget_show_all (mi);
	  g_object_unref(pixbuf);
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

  g_slist_free (list);

  if (pos == 0)
    {
      /* Insert empty menu item */
      mi = gtk_menu_item_new_with_label (_("Clipboard is empty"));
      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, 0);
      gtk_widget_set_sensitive (mi, FALSE);
      gtk_widget_show (mi);

      /* Set the clear history item insensitive */
      gtk_widget_set_sensitive (menu->priv->mi_clear_history, FALSE);
    }

  _clipman_menu_adjust_geometry(menu);
}

static void
_clipman_menu_free_list (ClipmanMenu *menu)
{
  GSList *list;
  for (list = menu->priv->list; list != NULL; list = list->next)
    gtk_widget_destroy (GTK_WIDGET (list->data));
  g_slist_free (menu->priv->list);
  menu->priv->list = NULL;
}

/*
 * Public methods
 */

GtkWidget *
clipman_menu_new (void)
{
  return g_object_new (CLIPMAN_TYPE_MENU, NULL);
}

/*
 * GObject
 */

static void
clipman_menu_class_init (ClipmanMenuClass *klass)
{
  GObjectClass *object_class;

  g_type_class_add_private (klass, sizeof (ClipmanMenuPrivate));

  clipman_menu_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = clipman_menu_finalize;
  object_class->set_property = clipman_menu_set_property;
  object_class->get_property = clipman_menu_get_property;

  g_object_class_install_property (object_class, REVERSE_ORDER,
                                   g_param_spec_boolean ("reverse-order",
                                                         "ReverseOrder",
                                                         "Set to TRUE to display the menu in the reverse order",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

#ifdef HAVE_QRENCODE
  g_object_class_install_property (object_class, SHOW_QR_CODE,
                                   g_param_spec_boolean ("show-qr-code",
                                                         "ShowQrCode",
                                                         "Set to TRUE to display QR-Code in the menu",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
#endif

  g_object_class_install_property (object_class, PASTE_ON_ACTIVATE,
                                   g_param_spec_uint ("paste-on-activate",
                                                      "PasteOnActivate",
                                                      "Paste the content of a menu item when it is activated",
                                                      0, 2, 0,
                                                      G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

  g_object_class_install_property (object_class, NEVER_CONFIRM_HISTORY_CLEAR,
                                   g_param_spec_boolean ("never-confirm-history-clear",
                                                         "NeverConfirmHistoryClear",
                                                         "Set to FALSE to clear the history list with confirmation",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
}

static void
clipman_menu_init (ClipmanMenu *menu)
{
  GtkWidget *mi;
  GtkWidget *image;

  menu->priv = GET_PRIVATE (menu);

  /* ClipmanHistory */
  menu->priv->history = clipman_history_get ();

  /* Connect signal on show to update the items */
  g_signal_connect_swapped (menu, "show", G_CALLBACK (_clipman_menu_update_list), menu);

  /* Footer items */
  mi = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  menu->priv->mi_clear_history = mi = gtk_image_menu_item_new_with_mnemonic (_("_Clear history"));
  image = gtk_image_new_from_icon_name ("edit-clear-symbolic", GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu->priv->mi_clear_history), image);
G_GNUC_END_IGNORE_DEPRECATIONS
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (mi, "activate", G_CALLBACK (cb_clear_history), menu);

  /* Show all the items */
  gtk_widget_show_all (GTK_WIDGET (menu));
}

static void
clipman_menu_finalize (GObject *object)
{
  _clipman_menu_free_list (CLIPMAN_MENU (object));
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

    default:
      break;
    }
}

#ifdef HAVE_QRENCODE
GdkPixbuf *
clipman_menu_qrcode (char *text)
{
	QRcode *qrcode;
	GdkPixbuf *pixbuf, *pixbuf_scaled;
	int i, j, k, rowstride, channels;
	guchar *pixel;
	unsigned char *data;

	qrcode = QRcode_encodeString8bit(text, 0, QR_ECLEVEL_L);

	if (qrcode == NULL)
		return NULL;

	data = qrcode->data;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, qrcode->width + 2, qrcode->width + 2);

	pixel = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	channels = gdk_pixbuf_get_n_channels (pixbuf);

	gdk_pixbuf_fill(pixbuf, 0xffffffff);
	for (i = 1; i <= qrcode->width; i++)
		for (j = 1; j <= qrcode->width; j++) {
			for (k = 0; k < channels; k++)
				pixel[i * rowstride + j * channels + k] = !(*data & 0x1) * 0xff;
			data++;
		}

	pixbuf_scaled = gdk_pixbuf_scale_simple (pixbuf, (qrcode->width + 2) * 3, (qrcode->width + 2) * 3, GDK_INTERP_NEAREST);

	QRcode_free(qrcode);
	g_object_unref(pixbuf);

	return pixbuf_scaled;
}
#endif
