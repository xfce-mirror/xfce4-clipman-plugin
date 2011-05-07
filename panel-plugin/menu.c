/*
 *  Copyright (c) 2008-2011 Mike Massonnet <mmassonnet@xfce.org>
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
#include <libxfce4util/libxfce4util.h>

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
  GtkWidget            *mi_inhibit;
  GtkWidget            *mi_clear_history;
  ClipmanHistory       *history;
  GSList               *list;
  gboolean              reverse_order;
};

enum
{
  REVERSE_ORDER = 1,
  INHIBIT_MENU_ITEM,
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

/*
 * Private methods declarations
 */

static void            _clipman_menu_free_list          (ClipmanMenu *menu);

/*
 * Callbacks declarations
 */

static void             cb_set_clipboard                (const ClipmanHistoryItem *item);
static void             cb_clear_history                (ClipmanMenu *menu);
static void             cb_toggle_inhibit_mi            (ClipmanMenu *menu);



/*
 * Callbacks
 */

static void
cb_set_clipboard (const ClipmanHistoryItem *item)
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
      g_assert_not_reached ();
    }
}

static void
cb_clear_history (ClipmanMenu *menu)
{
  GtkClipboard *clipboard;

  if (!xfce_dialog_confirm (NULL, GTK_STOCK_YES, NULL, _("Are you sure you want to clear the history?"), NULL))
    return;

  clipman_history_clear (menu->priv->history);

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_clear (clipboard);

  clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  gtk_clipboard_clear (clipboard);
}

static void
cb_toggle_inhibit_mi (ClipmanMenu *menu)
{
  gboolean toggle_value = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menu->priv->mi_inhibit));
  g_object_set (menu, "inhibit-menu-item", toggle_value, NULL);
}

/*
 * Private methods
 */

static void
_clipman_menu_update_list (ClipmanMenu *menu)
{
  GtkWidget *mi, *image;
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
          mi = gtk_image_menu_item_new_with_label (item->preview.text);
          g_signal_connect_swapped (mi, "activate", G_CALLBACK (cb_set_clipboard), item);
          break;

        case CLIPMAN_HISTORY_TYPE_IMAGE:
          mi = gtk_image_menu_item_new ();
          image = gtk_image_new_from_pixbuf (item->preview.image);
          gtk_container_add (GTK_CONTAINER (mi), image);
          g_signal_connect_swapped (mi, "activate", G_CALLBACK (cb_set_clipboard), item);
          break;

        default:
          g_assert_not_reached ();
        }

      if (item == item_to_restore)
        {
          image = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
        }

      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
      gtk_widget_show_all (mi);
    }
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

  g_object_class_install_property (object_class, INHIBIT_MENU_ITEM,
                                   g_param_spec_boolean ("inhibit-menu-item",
                                                         "InhibitMenuItem",
                                                         "Toggle the inhibit menu item to TRUE or FALSE",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
}

static void
clipman_menu_init (ClipmanMenu *menu)
{
  GtkWidget *mi;

  menu->priv = GET_PRIVATE (menu);

  /* ClipmanHistory */
  menu->priv->history = clipman_history_get ();

  /* Connect signal on show to update the items */
  g_signal_connect_swapped (menu, "show", G_CALLBACK (_clipman_menu_update_list), menu);

  /* Footer items */
  mi = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

  menu->priv->mi_inhibit = mi = gtk_check_menu_item_new_with_mnemonic (_("_Disable"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (mi, "toggled", G_CALLBACK (cb_toggle_inhibit_mi), menu);

  menu->priv->mi_clear_history = mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLEAR, NULL);
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

    case INHIBIT_MENU_ITEM:
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (priv->mi_inhibit),
                                      g_value_get_boolean (value));
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

    case INHIBIT_MENU_ITEM:
      g_value_set_boolean (value, gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (priv->mi_inhibit)));
      break;

    default:
      break;
    }
}

