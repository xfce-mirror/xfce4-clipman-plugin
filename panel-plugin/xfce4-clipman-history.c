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

#include "common.h"
#include "history.h"
#include "plugin.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>


enum
{
  COLUMN_PREVIEW,
  COLUMN_TEXT,
  N_COLUMNS
};

guint internal_paste_on_activate = PASTE_INACTIVE;


GtkWidget *
clipman_history_dialog_init (MyPlugin *plugin);
GtkWidget *
clipman_history_treeview_init (MyPlugin *plugin);
static void
clipman_history_copy_or_paste_on_activate (MyPlugin *plugin,
                                           guint paste_on_activate);



static void
clipman_history_row_activated (GtkTreeView *treeview,
                               GtkTreePath *path,
                               GtkTreeViewColumn *column,
                               MyPlugin *plugin)
{
  GtkWidget *window;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GDBusProxy *proxy;
  GError *error = NULL;
  gboolean ret;
  gchar *text;

  ret = gtk_tree_model_get_iter (gtk_tree_view_get_model (treeview), &iter, path);
  if (!ret)
    return;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter,
                      COLUMN_TEXT, &text,
                      -1);

  /* set clipboard text from the daemon instance */
  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES
                                           | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                                         NULL, "org.xfce.clipman", "/org/xfce/clipman",
                                         "org.gtk.Actions", NULL, &error);
  if (proxy != NULL)
    {
      GVariantBuilder *builder1 = g_variant_builder_new (G_VARIANT_TYPE ("av"));
      GVariantBuilder *builder2 = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
      GVariant *value;

      g_variant_builder_add (builder1, "v", g_variant_new_string (text));
      value = g_dbus_proxy_call_sync (proxy, "org.gtk.Actions.Activate",
                                      g_variant_new ("(sava{sv})", "set-text", builder1, builder2),
                                      G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
      g_variant_builder_unref (builder1);
      g_variant_builder_unref (builder2);

      if (value != NULL)
        g_variant_unref (value);
      else
        {
          g_warning ("Failed to activate set-text action on org.xfce.clipman: %s", error->message);
          g_error_free (error);
        }
    }
  else
    {
      g_warning ("Failed to get D-Bus proxy for org.xfce.clipman: %s", error->message);
      g_error_free (error);
    }

  window = gtk_widget_get_toplevel (GTK_WIDGET (treeview));
  if (GTK_IS_WINDOW (window))
    gtk_dialog_response (GTK_DIALOG (window), GTK_RESPONSE_CLOSE);

  g_free (text);
}

static void
clipman_history_search_entry_activate (GtkEntry *entry,
                                       MyPlugin *plugin)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *column;
  GtkTreePath *path;

  /* Make sure something is always selected in the treeview */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (plugin->treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  path = gtk_tree_model_get_path (model, &iter);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (plugin->treeview), COLUMN_PREVIEW);
  clipman_history_row_activated (GTK_TREE_VIEW (plugin->treeview), path, column, plugin);
}

static gboolean
clipman_history_visible_func (GtkTreeModel *model,
                              GtkTreeIter *iter,
                              gpointer user_data)
{
  GtkEntry *entry = GTK_ENTRY (user_data);
  const gchar *text;
  gchar *name;
  gchar *normalized;
  gchar *text_casefolded;
  gchar *name_casefolded;
  gboolean visible = FALSE;

  /* search string from dialog */
  text = gtk_entry_get_text (entry);
  if (text == NULL || text[0] == '\0')
    return TRUE;

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

static void
clipman_history_treeview_filter_and_select (MyPlugin *plugin)
{
  GtkTreePath *path;
  GtkTreeSelection *selection;
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->treeview));
  gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));

  path = gtk_tree_path_new_from_indices (0, -1);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (plugin->treeview));
  gtk_tree_selection_select_path (selection, path);

  if (gtk_tree_selection_path_is_selected (selection, path))
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (plugin->treeview), path, NULL, FALSE, 0.0, 0.0);

  gtk_tree_path_free (path);
}

static gboolean
clipman_history_key_event (GtkWidget *widget,
                           GdkEvent *event,
                           MyPlugin *plugin)
{
  GdkModifierType state = 0;
  gint ctrl_mask = 0;
  GdkDisplay *display = gdk_display_get_default ();

  GdkSeat *seat = gdk_display_get_default_seat (display);
  GdkDevice *device = gdk_seat_get_pointer (seat);
  GdkScreen *screen = gdk_screen_get_default ();
  GdkWindow *root_win = gdk_screen_get_root_window (screen);

  /* Check if the user is holding the Ctrl key and update Copy/Paste button accordingly */
  gdk_window_get_device_position (root_win, device, NULL, NULL, &state);
  ctrl_mask = state & GDK_CONTROL_MASK;
  if (ctrl_mask == GDK_CONTROL_MASK)
    {
      if (((GdkEventKey *) event)->type == GDK_KEY_RELEASE
          && (((GdkEventKey *) event)->keyval == GDK_KEY_Return
              || ((GdkEventKey *) event)->keyval == GDK_KEY_KP_Enter))
        {
          clipman_history_search_entry_activate (GTK_ENTRY (plugin->entry), plugin);
          return TRUE;
        }
      else
        clipman_history_copy_or_paste_on_activate (plugin, PASTE_INACTIVE);
    }
  else
    /* Anything else than PASTE_INACTIVE is fine here */
    clipman_history_copy_or_paste_on_activate (plugin, PASTE_CTRL_V);

  return FALSE;
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
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkWidget *entry, *scroll, *treeview, *box;

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start (box, 6);
  gtk_widget_set_margin_end (box, 6);
  gtk_widget_set_margin_top (box, 6);

  /* Create the search entry */
  plugin->entry = entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_widget_set_tooltip_text (entry, _("Enter search phrase here"));
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_PRIMARY, "edit-find");
  gtk_widget_show (entry);

  /* Scrolled Window */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand (scroll, TRUE);
  gtk_box_pack_start (GTK_BOX (box), scroll, TRUE, TRUE, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
  gtk_widget_show (scroll);

  /* Create the history liststore */
  liststore = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

  /* Create treemodel with filter */
  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (liststore), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter), clipman_history_visible_func, entry, NULL);
  g_signal_connect_swapped (G_OBJECT (entry), "changed", G_CALLBACK (clipman_history_treeview_filter_and_select), plugin);
  g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (clipman_history_search_entry_activate), plugin);

  /* Create the treeview */
  plugin->treeview = treeview = gtk_tree_view_new_with_model (filter);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeview), FALSE);
  g_signal_connect_swapped (G_OBJECT (treeview), "start-interactive-search", G_CALLBACK (gtk_widget_grab_focus), entry);
  g_signal_connect (G_OBJECT (treeview), "row-activated", G_CALLBACK (clipman_history_row_activated), plugin);

  gtk_container_add (GTK_CONTAINER (scroll), treeview);
  gtk_widget_show (treeview);

  g_object_unref (G_OBJECT (filter));
  gtk_list_store_clear (GTK_LIST_STORE (liststore));

  /* Add text renderer to visible column showing the text preview */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", COLUMN_PREVIEW,
                                       NULL);
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* Add text renderer to invisible column holding the full text */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_visible (column, FALSE);
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", COLUMN_TEXT,
                                       NULL);
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* Get the history and populate the treeview */
  list = clipman_history_get_list (plugin->history);
  if (list == NULL)
    {
      gtk_list_store_insert_with_values (liststore, &iter, 0,
                                         COLUMN_PREVIEW, _("History is empty"),
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
            /* We ignore everything but text (no images or QR codes) */
            case CLIPMAN_HISTORY_TYPE_TEXT:
              gtk_list_store_insert_with_values (liststore, &iter, i,
                                                 COLUMN_PREVIEW, item->preview.text,
                                                 COLUMN_TEXT, item->content.text,
                                                 -1);
              break;

            default:
              DBG ("Ignoring non-text history type %d", item->type);
              continue;
            }
        }

      g_slist_free (list);
    }

  /* Pre-select the first item in the list */
  path = gtk_tree_path_new_from_indices (0, -1);
  gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)), path);
  gtk_tree_path_free (path);

  return box;
}

static void
clipman_history_dialog_response (GtkWidget *dialog,
                                 gint response_id,
                                 MyPlugin *plugin)
{
  if (response_id == GTK_RESPONSE_HELP)
    xfce_dialog_show_help (GTK_WINDOW (dialog), "clipman", NULL, NULL);
  else if (response_id == GTK_RESPONSE_OK)
    plugin_configure (plugin);
  else if (response_id == GTK_RESPONSE_APPLY)
    clipman_history_search_entry_activate (GTK_ENTRY (plugin->entry), plugin);
  else
    gtk_widget_destroy (plugin->dialog);
}

static void
clipman_history_copy_or_paste_on_activate (MyPlugin *plugin,
                                           guint paste_on_activate)
{
  GtkWidget *icon;
  const gchar *button_text;

  g_return_if_fail (GTK_IS_WIDGET (plugin->submit_button));

  internal_paste_on_activate = paste_on_activate;

  if (paste_on_activate != PASTE_INACTIVE)
    {
      icon = gtk_image_new_from_icon_name ("edit-paste-symbolic", GTK_ICON_SIZE_BUTTON);
      button_text = _("_Paste");
    }
  else
    {
      icon = gtk_image_new_from_icon_name ("edit-copy-symbolic", GTK_ICON_SIZE_BUTTON);
      button_text = _("_Copy");
    }

  gtk_button_set_image (GTK_BUTTON (plugin->submit_button), icon);
  gtk_button_set_label (GTK_BUTTON (plugin->submit_button), button_text);
}

static void
clipman_history_paste_on_activate_changed (MyPlugin *plugin)
{
  internal_paste_on_activate = xfconf_channel_get_uint (plugin->channel, "/tweaks/paste-on-activate", PASTE_INACTIVE)
                               && WINDOWING_IS_X11 ();
  clipman_history_copy_or_paste_on_activate (plugin, internal_paste_on_activate);

  g_signal_handlers_disconnect_by_func (plugin->entry, clipman_history_key_event, plugin);
  g_signal_handlers_disconnect_by_func (plugin->treeview, clipman_history_key_event, plugin);
  if (internal_paste_on_activate != PASTE_INACTIVE)
    {
      g_signal_connect (G_OBJECT (plugin->entry), "key-press-event", G_CALLBACK (clipman_history_key_event), plugin);
      g_signal_connect (G_OBJECT (plugin->entry), "key-release-event", G_CALLBACK (clipman_history_key_event), plugin);
      g_signal_connect (G_OBJECT (plugin->treeview), "key-press-event", G_CALLBACK (clipman_history_key_event), plugin);
      g_signal_connect (G_OBJECT (plugin->treeview), "key-release-event", G_CALLBACK (clipman_history_key_event), plugin);
    }
}

GtkWidget *
clipman_history_dialog_init (MyPlugin *plugin)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *icon;

  dialog = xfce_titled_dialog_new ();
  gtk_window_set_application (GTK_WINDOW (dialog), GTK_APPLICATION (plugin->app));
  gtk_window_set_title (GTK_WINDOW (dialog), _("Clipman History"));
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-clipman-plugin");
  gtk_window_set_default_size (GTK_WINDOW (dialog), 350, 450);
  gtk_window_set_type_hint (GTK_WINDOW (dialog), GDK_WINDOW_TYPE_HINT_NORMAL);

#if !LIBXFCE4UI_CHECK_VERSION(4, 19, 3)
  xfce_titled_dialog_create_action_area (XFCE_TITLED_DIALOG (dialog));
#endif
  button = xfce_titled_dialog_add_button (XFCE_TITLED_DIALOG (dialog), _( "_Help"), GTK_RESPONSE_HELP);
  icon = gtk_image_new_from_icon_name ("help-browser", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), icon);

  button = xfce_titled_dialog_add_button (XFCE_TITLED_DIALOG (dialog), _( "_Settings"), GTK_RESPONSE_OK);
  icon = gtk_image_new_from_icon_name ("preferences-system", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), icon);

  plugin->submit_button = xfce_titled_dialog_add_button (XFCE_TITLED_DIALOG (dialog), "label", GTK_RESPONSE_APPLY);
  xfce_titled_dialog_set_default_response (XFCE_TITLED_DIALOG (dialog), GTK_RESPONSE_APPLY);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (plugin->submit_button)), "suggested-action");

  g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (clipman_history_dialog_response), plugin);

  return dialog;
}

static void
clipman_history_shutdown (GApplication *app,
                          gpointer user_data)
{
  MyPlugin *plugin = user_data;

  plugin_save (plugin);

  g_object_unref (plugin->channel);
  g_object_unref (plugin->history);
  g_slice_free (MyPlugin, plugin);

  xfconf_shutdown ();
}

static gint
clipman_history_command_line (GApplication *app,
                              GApplicationCommandLine *command_line,
                              gpointer user_data)
{
  MyPlugin *plugin;
  GtkWidget *box;
  GError *error = NULL;

  if (g_application_command_line_get_is_remote (command_line))
    {
      g_application_activate (app);
      return EXIT_SUCCESS;
    }

  if (!xfconf_init (&error))
    {
      g_critical ("Xfconf initialization failed: %s", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  plugin = g_slice_new0 (MyPlugin);
  plugin->app = GTK_APPLICATION (app);

  /* Bind all settings relevant for this application */
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
  g_signal_connect_swapped (plugin->channel, "property-changed::/tweaks/paste-on-activate",
                            G_CALLBACK (clipman_history_paste_on_activate_changed), plugin);

  /* Read the history from the cache file */
  plugin_load (plugin);

  /* Initialize dialog */
  plugin->dialog = clipman_history_dialog_init (plugin);
  g_signal_connect_swapped (app, "activate", G_CALLBACK (gtk_window_present), plugin->dialog);
  g_signal_connect (app, "shutdown", G_CALLBACK (clipman_history_shutdown), plugin);

  /* Initialize and pack history treeview */
  box = clipman_history_treeview_init (plugin);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (plugin->dialog))), box);
  gtk_widget_show_all (box);

  /* setup paste-on-activate */
  clipman_history_paste_on_activate_changed (plugin);

  gtk_widget_show_all (plugin->dialog);

  return EXIT_SUCCESS;
}

static gint
clipman_history_clipman_daemon_running (GApplication *app,
                                        GVariantDict *options,
                                        gpointer user_data)
{
  GError *error = NULL;

  if (!g_application_register (app, NULL, &error))
    {
      g_warning ("Unable to register GApplication: %s", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  if (!g_application_get_is_remote (app))
    {
      g_warning ("Unable to find the primary instance org.xfce.clipman");
      clipman_common_show_warning_dialog ();
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

gint
main (gint argc,
      gchar *argv[])
{
  GtkApplication *app;
  int status;

  /* Setup translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* check if clipman daemon is running */
  app = gtk_application_new ("org.xfce.clipman", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "handle-local-options", G_CALLBACK (clipman_history_clipman_daemon_running), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  if (status != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* run clipman-history app */
  app = gtk_application_new ("org.xfce.clipman.history", G_APPLICATION_HANDLES_COMMAND_LINE);
  g_signal_connect (app, "command-line", G_CALLBACK (clipman_history_command_line), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  /* Should be after gtk_widget_destroy() to assure focus being in the (previous) "target" window */
  if (internal_paste_on_activate != PASTE_INACTIVE)
    clipman_common_paste_on_activate (internal_paste_on_activate);

  return status;
}
