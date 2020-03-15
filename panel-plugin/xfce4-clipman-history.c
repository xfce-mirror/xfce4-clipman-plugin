/*
 *  Clipman - panel plugin for Xfce Desktop Environment
 *            command to show a dialog that allows to search the history
 *  Copyright (C) 2020  Simon Steinbeiß <simon@xfce.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <X11/extensions/XTest.h>

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <x11-clipboard-manager/daemon.h>

#include <plugin.h>
#include <history.h>


enum
{
  COLUMN_TEXT,
  N_COLUMNS
};


static gboolean
clipman_history_visible_func (GtkTreeModel *model,
                              GtkTreeIter  *iter,
                              gpointer      user_data)
{

  GtkEntry    *entry = GTK_ENTRY (user_data);
  const gchar *text;
  gchar       *name;
  gchar       *normalized;
  gchar       *text_casefolded;
  gchar       *name_casefolded;
  gboolean     visible = FALSE;

  /* search string from dialog */
  text = gtk_entry_get_text (entry);
  // FIXME:
  //if (G_UNLIKELY (panel_str_is_empty (text)))
  //    return TRUE;

  gtk_tree_model_get (model, iter, COLUMN_TEXT, &name, -1);

  /* casefold the search text */
  normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
  text_casefolded = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  if (G_LIKELY (name != NULL))
    {
      /* casefold the name */
      normalized = g_utf8_normalize (name, -1, G_NORMALIZE_ALL);
      name_casefolded = g_utf8_casefold (normalized, -1);
      g_free (normalized);

      /* search */
      visible = (strstr (name_casefolded, text_casefolded) != NULL);

      g_free (name_casefolded);
    }

  g_free (text_casefolded);

  return visible;
}

GtkWidget *
clipman_history_treeview_init (MyPlugin *plugin)
{
  ClipmanHistoryItem *item;
  gint i;
  GSList *list, *l;
  GtkTreeModel *filter;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *liststore;
  GtkTreeIter  iter;
  GtkWidget *entry, *scroll, *treeview, *label, *box;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  /* create the search entry */
  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 6);
  gtk_widget_set_tooltip_text (entry, _("Enter search phrase here"));
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_PRIMARY, "edit-find");
  gtk_widget_show (entry);

  /* scroller */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand (scroll, TRUE);
  gtk_box_pack_start (GTK_BOX (box), scroll, TRUE, TRUE, 6);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
  gtk_widget_show (scroll);

  /* create the store */
  liststore = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING);

  /* create treemodel with filter */
  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter), clipman_history_visible_func, entry, NULL);
  g_signal_connect_swapped (G_OBJECT (entry), "changed", G_CALLBACK (gtk_tree_model_filter_refilter), filter);

  /* create the treeview */
  treeview = gtk_tree_view_new_with_model (filter);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeview), FALSE);
  g_signal_connect_swapped (G_OBJECT (treeview), "start-interactive-search", G_CALLBACK (gtk_widget_grab_focus), entry);
  gtk_container_add (GTK_CONTAINER (scroll), treeview);
  gtk_widget_show (treeview);

  g_object_unref (G_OBJECT (filter));
  gtk_list_store_clear (GTK_LIST_STORE (liststore));

  /* text renderer */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", COLUMN_TEXT,
                                       NULL);
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  plugin->history = clipman_history_get ();
  list = clipman_history_get_list (plugin->history);
  //list = g_slist_reverse (list);

  if (list != NULL)
    g_warning ("startin the loop");
  for (l = list, i = 0; l != NULL; l = l->next, i++)
    {
      item = l->data;

      switch (item->type)
        {
        case CLIPMAN_HISTORY_TYPE_TEXT:
          gtk_list_store_insert_with_values (liststore, &iter, i, COLUMN_TEXT, item->preview.text, -1);
          break;

        case CLIPMAN_HISTORY_TYPE_IMAGE:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
//          mi = gtk_image_menu_item_new ();
//          image = gtk_image_new_from_pixbuf (item->preview.image);
//          gtk_container_add (GTK_CONTAINER (mi), image);
G_GNUC_END_IGNORE_DEPRECATIONS
          break;

        default:
          DBG("Ignoring unknown history type %d", item->type);
          continue;
        }

//      g_signal_connect (mi, "activate", G_CALLBACK (cb_set_clipboard), item);
//      g_object_set_data (G_OBJECT (mi), "paste-on-activate", GUINT_TO_POINTER (menu->priv->paste_on_activate));
//
//      if (item == item_to_restore)
//        {
//          image = gtk_image_new_from_icon_name ("go-next-symbolic", GTK_ICON_SIZE_MENU);
//G_GNUC_BEGIN_IGNORE_DEPRECATIONS
//          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
//G_GNUC_END_IGNORE_DEPRECATIONS
//        }

      //menu->priv->list = g_slist_prepend (menu->priv->list, mi);
//      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
//      gtk_widget_show_all (mi);
    }

#ifdef HAVE_QRENCODE
  /* Draw QR Code if clipboard content is text */
//  if (menu->priv->show_qr_code && item_to_restore && item_to_restore->type == CLIPMAN_HISTORY_TYPE_TEXT)
//    {
//      mi = gtk_separator_menu_item_new ();
//      menu->priv->list = g_slist_prepend (menu->priv->list, mi);
//      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
//      gtk_widget_show_all (mi);
//
//      if ((pixbuf = clipman_menu_qrcode (item_to_restore->content.text)) != NULL)
//        {
//G_GNUC_BEGIN_IGNORE_DEPRECATIONS
//          mi = gtk_image_menu_item_new ();
//G_GNUC_END_IGNORE_DEPRECATIONS
//          gtk_container_add (GTK_CONTAINER (mi), gtk_image_new_from_pixbuf (pixbuf));
//          g_signal_connect (mi, "activate", G_CALLBACK (cb_set_qrcode), pixbuf);
//          menu->priv->list = g_slist_prepend (menu->priv->list, mi);
//          gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
//          gtk_widget_show_all (mi);
//    g_object_unref(pixbuf);
//        }
//      else
//        {
//          mi = gtk_menu_item_new_with_label (_("Could not generate QR-Code."));
//          menu->priv->list = g_slist_prepend (menu->priv->list, mi);
//          gtk_menu_shell_insert (GTK_MENU_SHELL (menu), mi, pos++);
//          gtk_widget_set_sensitive (mi, FALSE);
//          gtk_widget_show (mi);
//        }
//    }
#endif

  g_slist_free (list);

  return box;
}

GtkWidget *
clipman_history_dialog_init (MyPlugin *plugin)
{
  GtkWidget *dialog;
  GtkWidget *box;
  GtkWidget *label;

  dialog = xfce_titled_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Clipboard History"));
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "clipman");
  gtk_window_set_default_size (GTK_WINDOW (dialog), 350, 450);
  gtk_window_set_type_hint (GTK_WINDOW (dialog), GDK_WINDOW_TYPE_HINT_NORMAL);

  box = clipman_history_treeview_init (plugin);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), box);
  gtk_widget_show_all (box);

  return dialog;
}

gint
main (gint argc, gchar *argv[])
{
  GtkApplication *app;
  GError *error = NULL;
  GtkWidget *dialog;
  MyPlugin *plugin = g_slice_new0 (MyPlugin);

  /* Setup translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  app = gtk_application_new ("org.xfce.clipman.history", 0);
  g_signal_connect_swapped (app, "activate", G_CALLBACK (gtk_window_present), dialog);
  g_application_run (G_APPLICATION (app), argc, argv);

  g_application_register (G_APPLICATION (app), NULL, &error);
  if (error != NULL)
    {
      g_warning ("Unable to register GApplication: %s", error->message);
      g_error_free (error);
      error = NULL;
    }

  if (g_application_get_is_remote (G_APPLICATION (app)))
    {
      g_application_activate (G_APPLICATION (app));
      g_object_unref (app);
      return FALSE;
    }

  xfconf_init (NULL);
  plugin->channel = xfconf_channel_new_with_property_base ("xfce4-panel", "/plugins/clipman");
  plugin->history = clipman_history_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/max-texts-in-history",
                          G_TYPE_UINT, plugin->history, "max-texts-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/max-images-in-history",
                          G_TYPE_UINT, plugin->history, "max-images-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/save-on-quit",
                          G_TYPE_BOOLEAN, plugin->history, "save-on-quit");
  xfconf_g_property_bind (plugin->channel, "/tweaks/reorder-items",
                          G_TYPE_BOOLEAN, plugin->history, "reorder-items");

  plugin_load (plugin);

  dialog = clipman_history_dialog_init (plugin);
  g_signal_connect (G_OBJECT (dialog), "delete-event", G_CALLBACK (gtk_main_quit), NULL);
  gtk_window_present (GTK_WINDOW (dialog));
  gtk_main ();

  return FALSE;
}
