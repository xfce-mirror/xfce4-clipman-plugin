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

#include <menu.h>
#include <plugin.h>
#include <history.h>


enum
{
  COLUMN_PREVIEW,
  COLUMN_TEXT,
  N_COLUMNS
};


static void
clipman_history_row_activated (GtkTreeView       *treeview,
                               GtkTreePath       *path,
                               GtkTreeViewColumn *column,
                               MyPlugin          *plugin)
{
  GtkWidget *window;
  GtkClipboard *clipboard;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean ret;
  gchar *text;
  guint paste_on_activate;

  ret = gtk_tree_model_get_iter (gtk_tree_view_get_model (treeview), &iter, path);
  if (!ret)
    return;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter,
                      COLUMN_TEXT, &text,
                      -1);

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (clipboard, text, -1);

  clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  gtk_clipboard_set_text (clipboard, text, -1);

  window = gtk_widget_get_toplevel (GTK_WIDGET (treeview));

//  g_object_get (G_OBJECT (plugin->menu), "paste-on-activate", &paste_on_activate, NULL);
//  if (paste_on_activate > 0)
//    {
//      g_warning ("close the window and paste.,..");
//      gtk_window_iconify (GTK_WINDOW (window));
//      g_usleep (1000000);
//      cb_paste_on_activate (paste_on_activate);
//      //gtk_window_deiconify (GTK_WINDOW (window));
//    }

  if (GTK_IS_WINDOW (window))
    gtk_dialog_response (GTK_DIALOG (window), GTK_RESPONSE_CLOSE);
}

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
  gboolean reverse_order = FALSE;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start (box, 6);
  gtk_widget_set_margin_end (box, 6);
  gtk_widget_set_margin_top (box, 6);

  /* create the search entry */
  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (entry, _("Enter search phrase here"));
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_PRIMARY, "edit-find");
  gtk_widget_show (entry);

  /* scroller */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand (scroll, TRUE);
  gtk_box_pack_start (GTK_BOX (box), scroll, TRUE, TRUE, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
  gtk_widget_show (scroll);

  /* create the store */
  liststore = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

  /* create treemodel with filter */
  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter), clipman_history_visible_func, entry, NULL);
  g_signal_connect_swapped (G_OBJECT (entry), "changed", G_CALLBACK (gtk_tree_model_filter_refilter), filter);

  /* create the treeview */
  treeview = gtk_tree_view_new_with_model (filter);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeview), FALSE);
  g_signal_connect_swapped (G_OBJECT (treeview), "start-interactive-search", G_CALLBACK (gtk_widget_grab_focus), entry);
  g_signal_connect (G_OBJECT (treeview), "row-activated", G_CALLBACK (clipman_history_row_activated), plugin);
  gtk_container_add (GTK_CONTAINER (scroll), treeview);
  gtk_widget_show (treeview);

  g_object_unref (G_OBJECT (filter));
  gtk_list_store_clear (GTK_LIST_STORE (liststore));

  /* text renderer */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", COLUMN_PREVIEW,
                                       NULL);
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* text renderer */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_visible (column, FALSE);
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", COLUMN_TEXT,
                                       NULL);
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  plugin->history = clipman_history_get ();
  list = clipman_history_get_list (plugin->history);

  g_object_get (G_OBJECT (plugin->menu), "reverse-order", &reverse_order, NULL);
  if (reverse_order)
    list = g_slist_reverse (list);

  if (list == NULL)
    {
      gtk_list_store_insert_with_values (liststore, &iter, 0,
                                         COLUMN_PREVIEW, _("Clipboard is empty"),
                                         COLUMN_TEXT, "",
                                         -1);
    }
  else
    {
      for (l = list, i = 0; l != NULL; l = l->next, i++)
        {
          item = l->data;

          switch (item->type)
            {
            case CLIPMAN_HISTORY_TYPE_TEXT:
              gtk_list_store_insert_with_values (liststore, &iter, i,
                                                 COLUMN_PREVIEW, item->preview.text,
                                                 COLUMN_TEXT, item->content.text,
                                                 -1);
              break;

            default:
              DBG("Ignoring non-text history type %d", item->type);
              continue;
            }
        }

      g_slist_free (list);
    }


  return box;
}

GtkWidget *
clipman_history_dialog_init (MyPlugin *plugin)
{
  GtkWidget *dialog;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *icon;
  GtkWidget *button;

  dialog = xfce_titled_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Clipboard History"));
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-clipman-plugin");
  gtk_window_set_default_size (GTK_WINDOW (dialog), 350, 450);
  gtk_window_set_type_hint (GTK_WINDOW (dialog), GDK_WINDOW_TYPE_HINT_NORMAL);

#if LIBXFCE4UI_CHECK_VERSION (4,15,0)
  xfce_titled_dialog_create_action_area (XFCE_TITLED_DIALOG (dialog));
  button = xfce_titled_dialog_add_button (XFCE_TITLED_DIALOG (dialog), _("_Close"), GTK_RESPONSE_CLOSE);
  xfce_titled_dialog_set_default_response (XFCE_TITLED_DIALOG (dialog), GTK_RESPONSE_CLOSE);
#else
  button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Close"), GTK_RESPONSE_CLOSE);
  icon = gtk_image_new_from_icon_name ("window-close-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), icon);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
#endif

  box = clipman_history_treeview_init (plugin);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), box);
  gtk_widget_show_all (box);

  return dialog;
}

static void
clipman_history_dialog_finalize (MyPlugin  *plugin,
                                 GtkWidget *window)
{
  guint paste_on_activate = 1;

  //g_object_get (G_OBJECT (plugin->menu), "paste-on-activate", &paste_on_activate, NULL);
  if (paste_on_activate > 0)
    {
      g_warning ("close the window and paste... %d", paste_on_activate);
      if (GTK_IS_WIDGET (window))
        gtk_widget_hide (window);
      while (gtk_widget_get_visible (window))
        g_usleep (1000000);
      cb_paste_on_activate (paste_on_activate);
      //gtk_window_deiconify (GTK_WINDOW (window));
    }

  plugin_save (plugin);
  g_application_quit(G_APPLICATION(plugin->app));
}

static void
clipman_history_dialog_response (GtkWidget *dialog,
                                 gint       response_id,
                                 MyPlugin  *plugin)
{
  if (response_id == GTK_RESPONSE_CLOSE)
    clipman_history_dialog_finalize (plugin, dialog);
}

gboolean
clipman_history_dialog_delete_event (GtkWidget *widget,
                                     GdkEvent  *event,
                                     MyPlugin  *plugin)
{
  clipman_history_dialog_finalize (plugin, widget);

  return TRUE;
}

/* dummy function as we don't want to activate */
static void
activate (GApplication *app, gpointer      user_data)
{
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

  app = gtk_application_new ("org.xfce.clipman.history", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
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

  plugin->app = app;
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

  plugin->menu = clipman_menu_new ();
  xfconf_g_property_bind (plugin->channel, "/tweaks/paste-on-activate",
                          G_TYPE_UINT, plugin->menu, "paste-on-activate");
  xfconf_g_property_bind (plugin->channel, "/tweaks/reverse-menu-order",
                          G_TYPE_BOOLEAN, plugin->menu, "reverse-order");


  plugin_load (plugin);

  dialog = clipman_history_dialog_init (plugin);
  g_signal_connect (G_OBJECT (dialog), "delete-event", G_CALLBACK (clipman_history_dialog_delete_event), plugin);
  g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (clipman_history_dialog_response), plugin);
  gtk_window_present (GTK_WINDOW (dialog));
  gtk_dialog_run(GTK_DIALOG(dialog));

  return FALSE;
}
