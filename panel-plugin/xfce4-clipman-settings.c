/*
 *  Copyright (c) 2009 Mike Massonnet <mmassonnet@xfce.org>
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

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_UNIQUE
#include <unique/unique.h>
#endif

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include "common.h"
#include "settings-dialog_ui.h"
#include "actions.h"

static void             prop_dialog_run                 ();
static void             cb_show_help                    (GtkButton *button);
static void             setup_actions_treeview          (GtkTreeView *treeview);
static void             refresh_actions_treeview        (GtkTreeView *treeview);
static void             apply_action                    (const gchar *original_action_name);
static void             cb_actions_selection_changed    (GtkTreeSelection *selection);
static void             cb_add_action                   (GtkButton *button);
static void             cb_edit_action                  (GtkButton *button);
static void             cb_actions_row_activated        (GtkTreeView *treeview,
                                                         GtkTreePath *path,
                                                         GtkTreeViewColumn *column);
static void             cb_delete_action                (GtkButton *button);
static void             cb_reset_actions                (GtkButton *button);
static void             setup_commands_treeview         (GtkTreeView *treeview);
static void             entry_dialog_cleanup            ();
#if !GLIB_CHECK_VERSION(2,16,0)
static void           __foreach_command_fill_commands   (gpointer key,
                                                         gpointer value,
                                                         gpointer user_data);
#endif
static void             cb_commands_selection_changed   (GtkTreeSelection *selection);
static void             cb_add_command                  (GtkButton *button);
static void             cb_refresh_command              (GtkButton *button);
static void             cb_delete_command               (GtkButton *button);
static void             setup_test_regex_dialog         ();
static void             cb_test_regex                   (GtkButton *button);
static void             cb_test_regex_changed           (GtkWidget *widget);
static gboolean         cb_test_regex_changed_timeout   ();
static void             update_test_regex_textview_tags ();
static void             cb_set_action_dialog_button_ok  (GtkWidget *widget);

static XfconfChannel *xfconf_channel = NULL;
static GtkBuilder *builder = NULL;
static ClipmanActions *actions = NULL;
static GtkWidget *settings_dialog = NULL;
static guint test_regex_changed_timeout = 0;



static void
prop_dialog_run (void)
{
  GtkWidget *action_dialog;

  builder = gtk_builder_new ();
  gtk_builder_add_from_string (builder, settings_dialog_ui, settings_dialog_ui_length, NULL);

  /* Dialogs */
  settings_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "settings-dialog"));
  action_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "action-dialog"));

  /* General settings */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "add-selections")),
                                DEFAULT_ADD_PRIMARY_CLIPBOARD);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "history-ignore-selections")),
                                DEFAULT_HISTORY_IGNORE_PRIMARY_CLIPBOARD);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "save-on-quit")),
                                DEFAULT_SAVE_ON_QUIT);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "store-an-image")),
                                (gboolean)DEFAULT_MAX_IMAGES_IN_HISTORY);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "max-texts-in-history")),
                             (gdouble)DEFAULT_MAX_TEXTS_IN_HISTORY);

  xfconf_g_property_bind (xfconf_channel, "/settings/add-primary-clipboard", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "add-selections"), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/history-ignore-primary-clipboard", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "history-ignore-selections"), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "save-on-quit"), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/max-images-in-history", G_TYPE_UINT,
                          gtk_builder_get_object (builder, "store-an-image"), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/max-texts-in-history", G_TYPE_UINT,
                          gtk_builder_get_object (builder, "max-texts-in-history"), "value");

  /* Actions tab and dialog */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "enable-actions")),
                                DEFAULT_ENABLE_ACTIONS);
  xfconf_g_property_bind (xfconf_channel, "/settings/enable-actions", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "enable-actions"), "active");

  g_signal_connect (gtk_builder_get_object (builder, "button-add-action"), "clicked", G_CALLBACK (cb_add_action), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-edit-action"), "clicked", G_CALLBACK (cb_edit_action), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-delete-action"), "clicked", G_CALLBACK (cb_delete_action), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-reset-actions"), "clicked", G_CALLBACK (cb_reset_actions), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "actions"), "row_activated", G_CALLBACK (cb_actions_row_activated), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-add-command"), "clicked", G_CALLBACK (cb_add_command), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-refresh-command"), "clicked", G_CALLBACK (cb_refresh_command), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-delete-command"), "clicked", G_CALLBACK(cb_delete_command), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "settings-dialog-button-help"), "clicked", G_CALLBACK (cb_show_help), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-action-pattern"), "clicked", G_CALLBACK (cb_test_regex), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "regex-entry"), "changed", G_CALLBACK (cb_test_regex_changed), NULL);

  g_signal_connect_swapped (gtk_builder_get_object (builder, "settings-dialog-button-close"), "clicked",
                             G_CALLBACK (gtk_dialog_response), settings_dialog);

  setup_actions_treeview (GTK_TREE_VIEW (gtk_builder_get_object (builder, "actions")));
  setup_commands_treeview (GTK_TREE_VIEW (gtk_builder_get_object (builder, "commands")));
  setup_test_regex_dialog ();

  /* Callbacks for the OK button sensitivity in the edit action dialog */
  g_signal_connect_after (gtk_builder_get_object (builder, "action-name"), "changed",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (gtk_builder_get_object (builder, "regex"), "changed",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (gtk_builder_get_object (builder, "button-add-command"), "clicked",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (gtk_builder_get_object (builder, "button-delete-command"), "clicked",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);

  g_signal_connect_after (gtk_builder_get_object (builder, "button-delete-command"), "clicked",
                          G_CALLBACK (gtk_dialog_response), NULL);

  g_signal_connect_swapped (gtk_builder_get_object (builder, "action-dialog-button-cancel"), "clicked",
                             G_CALLBACK (gtk_dialog_response), action_dialog);

  /* Run the dialog */
  while ((gtk_dialog_run (GTK_DIALOG (settings_dialog))) == 2);

  gtk_widget_destroy (action_dialog);
  gtk_widget_destroy (settings_dialog);
  g_object_unref (G_OBJECT(builder));

  /* Save the actions */
  clipman_actions_save (actions);
}

static void
cb_show_help (GtkButton *button)
{
  GdkScreen *screen;
  gchar *locale = NULL;
  gchar *offset;
  gchar *filename = NULL;
  gchar *command = NULL;
  
#ifdef ENABLE_NLS
#ifdef HAVE_LOCALE_H
  locale = g_strdup (setlocale (LC_MESSAGES, ""));
  if (locale != NULL)
    {
      offset = g_strrstr (locale, ".");
      if (offset != NULL)
        *offset = '\0';
    }
  else
    locale = g_strdup ("C");
#else
  locale = g_strdup ("C");
#endif

  filename = g_strdup_printf (DATAROOTDIR"/xfce4/doc/%s/"PACKAGE".html", locale);
  if (!g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      offset = g_strrstr (locale, "_");
      if (offset == NULL)
        {
          g_free (filename);
          filename = g_strdup (DATAROOTDIR"/xfce4/doc/C/"PACKAGE".html");
        }
      else
        {
          *offset = '\0';
          g_free (filename);
          filename = g_strdup_printf (DATAROOTDIR"/xfce4/doc/%s/"PACKAGE".html", locale);
          if (!g_file_test (filename, G_FILE_TEST_EXISTS))
            {
              g_free (filename);
              filename = g_strdup (DATAROOTDIR"/xfce4/doc/C/"PACKAGE".html");
            }
        }
    }

  g_free (locale);
#else
  filename = g_strdup (DATAROOTDIR"/xfce4/doc/C/"PACKAGE".html");
#endif

  screen = gtk_widget_get_screen (GTK_WIDGET (button));
  command = g_strdup_printf ("exo-open file://%s", filename);
  if (gdk_spawn_command_line_on_screen (screen, command, NULL))
    goto out;

  g_free (command);
  command = g_strdup_printf ("firefox file://%s", filename);
  if (gdk_spawn_command_line_on_screen (screen, command, NULL))
    goto out;

  xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
                          NULL, "Unable to open documentation \"%s\"", filename);

out:
  g_free (filename);
  g_free (command);
}



/* Actions */
static void
setup_actions_treeview (GtkTreeView *treeview)
{
  GtkTreeSelection *selection;
  GtkListStore *model;
  GtkCellRenderer *cell;

  /* Define the model */
  model = gtk_list_store_new (2, G_TYPE_POINTER, G_TYPE_STRING);
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (model));
  g_object_unref (model);

  /* Define the columns */
  cell = gtk_cell_renderer_text_new ();
  g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_insert_column_with_attributes (treeview, -1, "Action", cell, "markup", 1, NULL);

  refresh_actions_treeview (treeview);

  selection = gtk_tree_view_get_selection (treeview);
  g_signal_connect (selection, "changed", G_CALLBACK (cb_actions_selection_changed), NULL);
}

static void
refresh_actions_treeview (GtkTreeView *treeview)
{
  ClipmanActionsEntry *entry;
  const GSList *entries;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *title;

  model = gtk_tree_view_get_model (treeview);
  gtk_list_store_clear (GTK_LIST_STORE (model));

  entries = clipman_actions_get_entries (actions);
  for (; entries != NULL; entries = entries->next)
    {
      entry = entries->data;

      title = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>",
                                       entry->action_name, entry->pattern);
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, entry, 1, title, -1);
      g_free (title);
    }
}

static void
apply_action (const gchar *original_action_name)
{
  GtkWidget *treeview;
  GtkTreeModel *model;
  GtkTreeIter iter;
  const gchar *action_name;
  const gchar *regex;
  gint group;
  gchar *command_name;
  gchar *command;

  action_name = gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "action-name")));
  regex = gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "regex")));
  group = (gint)gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "manual")));

  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "commands"));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_model_get_iter_first (model, &iter))
    return;

  /* Remove the old actions */
  if (original_action_name != NULL)
    clipman_actions_remove (actions, original_action_name);

  /* Add the new actions */
  do
    {
      gtk_tree_model_get (model, &iter, 1, &command_name, 2, &command, -1);
      clipman_actions_add (actions, action_name, regex, command_name, command);
      clipman_actions_set_group (actions, action_name, group);
      g_free (command_name);
      g_free (command);
    }
  while (gtk_tree_model_iter_next (model, &iter));

  /* Refresh the actions treeview */
  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "actions"));
  refresh_actions_treeview (GTK_TREE_VIEW (treeview));
}

static void
cb_actions_selection_changed (GtkTreeSelection *selection)
{
  GtkTreeModel *model;
  gboolean sensitive;

  sensitive = gtk_tree_selection_get_selected (selection, &model, NULL);

  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button-edit-action")), sensitive);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button-delete-action")), sensitive);
}

static void
cb_add_action (GtkButton *button)
{
  GtkWidget *action_dialog;
  gint res;

  action_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "action-dialog"));
  entry_dialog_cleanup ();

  res = gtk_dialog_run (GTK_DIALOG (action_dialog));
  gtk_widget_hide (action_dialog);

  if (res == 1)
    apply_action (NULL);
}

static void
cb_edit_action (GtkButton *button)
{
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "actions"));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      g_critical ("Trying to edit an action but got no selection");
      return;
    }

  path = gtk_tree_model_get_path (model, &iter);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), 1);
  gtk_tree_view_row_activated (GTK_TREE_VIEW (treeview), path, column);
  gtk_tree_path_free (path);
}

static void
cb_actions_row_activated (GtkTreeView *treeview,
                          GtkTreePath *path,
                          GtkTreeViewColumn *column)
{
  ClipmanActionsEntry *entry;
  GtkTreeModel *actions_model, *commands_model;
  GtkTreeIter iter;
  GtkWidget *action_dialog;
  gchar *title;
  gint res;

  action_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "action-dialog"));
  entry_dialog_cleanup ();

  actions_model = gtk_tree_view_get_model (treeview);
  gtk_tree_model_get_iter (actions_model, &iter, path);
  gtk_tree_model_get (actions_model, &iter, 0, &entry, -1);

  commands_model = gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "commands")));
#if GLIB_CHECK_VERSION (2,16,0)
    {
      GHashTableIter hiter;
      gpointer key, value;
      g_hash_table_iter_init (&hiter, entry->commands);
      while (g_hash_table_iter_next (&hiter, &key, &value))
        {
          title = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>", (gchar *)key, (gchar *)value);
          gtk_list_store_append (GTK_LIST_STORE (commands_model), &iter);
          gtk_list_store_set (GTK_LIST_STORE (commands_model), &iter, 0, title, 1, key, 2, value, -1);
          g_free (title);
        }
    }
#else
  g_hash_table_foreach (entry->commands, (GHFunc)__foreach_command_fill_commands, commands_model);
#endif

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "action-name")), entry->action_name);
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "regex")), entry->pattern);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "manual")), entry->group);

  res = gtk_dialog_run (GTK_DIALOG (action_dialog));
  gtk_widget_hide (action_dialog);

  if (res == 1)
    apply_action (entry->action_name);
}

static void
cb_delete_action (GtkButton *button)
{
  ClipmanActionsEntry *entry;
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "actions"));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      g_critical ("Trying to remove an action but got no selection");
      return;
    }

  gtk_tree_model_get (model, &iter, 0, &entry, -1);
  clipman_actions_remove (actions, entry->action_name);
  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

static void
cb_reset_actions (GtkButton *button)
{
  GtkWidget *dialog;
  gchar *filename;
  gint res;

  dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (settings_dialog),
                                               GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               _("<b>Reset actions</b>"), NULL);
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            _("Are you sure you want to reset the actions to the system default values?"));
  res = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  if (res != GTK_RESPONSE_YES)
    return;

  filename = g_strdup_printf ("%s/xfce4/panel/xfce4-clipman-actions.xml", g_get_user_config_dir ());
  g_unlink (filename);
  g_free (filename);

  g_object_unref (actions);
  actions = clipman_actions_get ();
  refresh_actions_treeview (GTK_TREE_VIEW (gtk_builder_get_object (builder, "actions")));
}



/* Actions Entry */
static void
setup_commands_treeview (GtkTreeView *treeview)
{
  GtkTreeSelection *selection;
  GtkListStore *model;
  GtkCellRenderer *cell;

  /* Define the model */
  model = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (model));
  g_object_unref (model);

  /* Define the columns */
  cell = gtk_cell_renderer_text_new ();
  g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_insert_column_with_attributes (treeview, -1, "Command", cell, "markup", 0, NULL);

  selection = gtk_tree_view_get_selection (treeview);
  g_signal_connect (selection, "changed", G_CALLBACK (cb_commands_selection_changed), NULL);
}

static void
entry_dialog_cleanup (void)
{
  GtkTreeModel *model;

  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "action-dialog-button-ok")), FALSE);

  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "action-name")), "");
  gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "regex")), "");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "manual")), FALSE);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "commands")));
  gtk_list_store_clear (GTK_LIST_STORE (model));
}

#if !GLIB_CHECK_VERSION(2,16,0)
static void
__foreach_command_fill_commands (gpointer key,
                                 gpointer value,
                                 gpointer user_data)
{
  GtkTreeModel *model = user_data;
  GtkTreeIter iter;
  gchar *title;

  title = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>", (gchar *)key, (gchar *)value);
  gtk_list_store_append (GTK_LIST_STORE (_model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, title, 1, key, 2, value, -1);
  g_free (title);
}
#endif

static void
cb_commands_selection_changed (GtkTreeSelection *selection)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean sensitive;
  gchar *command_name = NULL;
  gchar *command = NULL;

  sensitive = gtk_tree_selection_get_selected (selection, &model, &iter);

  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button-refresh-command")), sensitive);
  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "button-delete-command")), sensitive);

  if (sensitive)
    {
      gtk_tree_model_get (model, &iter, 1, &command_name, 2, &command, -1);
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "command-name")), command_name);
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "command")), command);
      g_free (command_name);
      g_free (command);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "command-name")), "");
      gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "command")), "");
    }
}

static void
cb_add_command (GtkButton *button)
{
  GtkWidget *command_name;
  GtkWidget *command;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *title;

  command_name = GTK_WIDGET (gtk_builder_get_object (builder, "command-name"));
  command = GTK_WIDGET (gtk_builder_get_object (builder, "command"));

  if (gtk_entry_get_text (GTK_ENTRY (command_name))[0] == '\0'
      || gtk_entry_get_text (GTK_ENTRY (command))[0] == '\0')
    return;

  title = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>",
                                   gtk_entry_get_text (GTK_ENTRY (command_name)),
                                   gtk_entry_get_text (GTK_ENTRY (command)));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "commands")));
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, title,
                      1, gtk_entry_get_text (GTK_ENTRY (command_name)),
                      2, gtk_entry_get_text (GTK_ENTRY (command)), -1);
  g_free (title);

  gtk_entry_set_text (GTK_ENTRY (command_name), "");
  gtk_entry_set_text (GTK_ENTRY (command), "");
}

static void
cb_refresh_command (GtkButton *button)
{
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkWidget *command_name;
  GtkWidget *command;
  gchar *title;

  command_name = GTK_WIDGET (gtk_builder_get_object (builder, "command-name"));
  command = GTK_WIDGET (gtk_builder_get_object (builder, "command"));

  if (gtk_entry_get_text (GTK_ENTRY (command_name))[0] == '\0'
      || gtk_entry_get_text (GTK_ENTRY (command))[0] == '\0')
    return;

  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "commands"));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      g_critical ("Trying to refresh a command but got no selection");
      return;
    }

  title = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>",
                                   gtk_entry_get_text (GTK_ENTRY (command_name)),
                                   gtk_entry_get_text (GTK_ENTRY (command)));
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, title,
                      1, gtk_entry_get_text (GTK_ENTRY (command_name)),
                      2, gtk_entry_get_text (GTK_ENTRY (command)), -1);
  g_free (title);

  gtk_tree_selection_unselect_all (selection);
}

static void
cb_delete_command (GtkButton *button)
{
  GtkWidget *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  treeview = GTK_WIDGET (gtk_builder_get_object (builder, "commands"));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      g_critical ("Trying to delete a command but got no selection");
      return;
    }

  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}



/* Regex dialog */
static void
setup_test_regex_dialog (void)
{
  GtkWidget *textview;
  GtkTextBuffer *buffer;

  textview = GTK_WIDGET (gtk_builder_get_object (builder, "regex-textview"));
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));

  gtk_text_buffer_create_tag (buffer, "match",
                              "background", "#555152",
                              "foreground", "#EFFFCD",
                              NULL);
  gtk_text_buffer_create_tag (buffer, "match-secondary",
                              "background", "#2E2633",
                              "foreground", "#DCE9BE",
                              NULL);

  g_signal_connect (buffer, "changed", G_CALLBACK (cb_test_regex_changed), NULL);
}

static void
cb_test_regex (GtkButton *button)
{
  GtkWidget *regex;
  GtkWidget *dialog;
  GtkWidget *entry;
  const gchar *pattern;

  regex = GTK_WIDGET (gtk_builder_get_object (builder, "regex"));
  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "regex-dialog"));
  entry = GTK_WIDGET (gtk_builder_get_object (builder, "regex-entry"));

  pattern = gtk_entry_get_text (GTK_ENTRY (regex));
  gtk_entry_set_text (GTK_ENTRY (entry), pattern);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_APPLY)
    {
      pattern = gtk_entry_get_text (GTK_ENTRY (entry));
      gtk_entry_set_text (GTK_ENTRY (regex), pattern);
    }

  gtk_widget_hide (dialog);
}

static void 
cb_test_regex_changed (GtkWidget *widget)
{
#if GTK_CHECK_VERSION (2, 16, 0)
  if (test_regex_changed_timeout == 0)
    gtk_entry_set_icon_from_stock (GTK_ENTRY (gtk_builder_get_object (builder, "regex-entry")),
                                   GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_REFRESH);
#endif

  if (test_regex_changed_timeout > 0)
    g_source_remove (test_regex_changed_timeout);

  test_regex_changed_timeout = g_timeout_add_seconds (1, (GSourceFunc)cb_test_regex_changed_timeout, NULL);
}

static gboolean
cb_test_regex_changed_timeout (void)
{
  update_test_regex_textview_tags ();
  test_regex_changed_timeout = 0;
  return FALSE;
}

static void
update_test_regex_textview_tags (void)
{
  GtkWidget *entry;
  GtkWidget *textview;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  GRegex *regex;
  GMatchInfo *match_info = NULL;
  const gchar *pattern;
  gchar *text;

  entry = GTK_WIDGET (gtk_builder_get_object (builder, "regex-entry"));
  textview = GTK_WIDGET (gtk_builder_get_object (builder, "regex-textview"));
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
  gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
  gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);

  /* Remove all tags */
  gtk_text_buffer_remove_all_tags (buffer, &start, &end);

  /* Build Regex */
  pattern = gtk_entry_get_text (GTK_ENTRY (entry));
  regex = g_regex_new (pattern, G_REGEX_CASELESS|G_REGEX_MULTILINE, 0, NULL);
  if (regex == NULL)
    {
#if GTK_CHECK_VERSION (2, 16, 0)
      gtk_entry_set_icon_from_stock (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_ERROR);
#endif
      return;
    }
#if GTK_CHECK_VERSION (2, 16, 0)
  gtk_entry_set_icon_from_stock (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_APPLY);
#endif

  text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  if (!g_regex_match (regex, text, 0, &match_info))
    {
      g_regex_unref (regex);
      g_match_info_free (match_info);
      g_free (text);
      return;
    }

  /* Update tags in textview */
  do
    {
      gint i;
      gint match_count;

      match_count = g_match_info_get_match_count (match_info);
      for (i = 0; i < match_count; i++)
        {
          gint start_pos, end_pos;
          GtkTextIter tag_start, tag_end;

          /* Insert tag at pos start_pos,end_pos */
          g_match_info_fetch_pos (match_info, i, &start_pos, &end_pos);
          start_pos = g_utf8_pointer_to_offset (text, text + start_pos);
          end_pos = g_utf8_pointer_to_offset (text, text + end_pos);
          gtk_text_buffer_get_iter_at_offset (buffer, &tag_start, start_pos);
          gtk_text_buffer_get_iter_at_offset (buffer, &tag_end, end_pos);
          if (i == 0)
            gtk_text_buffer_apply_tag_by_name (buffer, "match", &tag_start, &tag_end);
          else
            gtk_text_buffer_apply_tag_by_name (buffer, "match-secondary", &tag_start, &tag_end);
        }
    }
  while  (g_match_info_next (match_info, NULL));

  g_regex_unref (regex);
  g_match_info_free (match_info);
  g_free (text);
}



/* Sensitivity of buttons */
static void
cb_set_action_dialog_button_ok (GtkWidget *widget)
{
  const gchar *action_name;
  const gchar *regex_pattern;
  GRegex *regex;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean has_commands;
  gboolean sensitive = FALSE;

  action_name = gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "action-name")));
  regex_pattern = gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "regex")));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (gtk_builder_get_object (builder, "commands")));
  has_commands = gtk_tree_model_get_iter_first (model, &iter);

  if (action_name[0] != '\0' && regex_pattern[0] != '\0' && has_commands)
    {
      regex = g_regex_new (regex_pattern, 0, 0, NULL);
      if (regex != NULL)
        {
          sensitive = TRUE;
          g_regex_unref (regex);
        }
    }

  gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "action-dialog-button-ok")), sensitive);
  return;
}



/* Main */
#ifdef HAVE_UNIQUE
static UniqueResponse
cb_unique_app (UniqueApp *app,
               gint command,
               UniqueMessageData *message_data,
               guint time_,
               gpointer user_data)
{
  if (command != UNIQUE_ACTIVATE)
    {
      return UNIQUE_RESPONSE_PASSTHROUGH;
    }

  gtk_window_present (GTK_WINDOW (settings_dialog));
  return UNIQUE_RESPONSE_OK;
}
#endif

gint
main (gint argc,
      gchar *argv[])
{
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, NULL);
  xfconf_init (NULL);
  gtk_init (&argc, &argv);

#ifdef HAVE_UNIQUE
    {
      UniqueApp *app = unique_app_new ("org.xfce.Clipman", NULL);
      if (unique_app_is_running (app))
        {
          if (unique_app_send_message (app, UNIQUE_ACTIVATE, NULL) == UNIQUE_RESPONSE_OK)
            {
              g_object_unref (app);
              return 0;
            }
        }
      g_signal_connect (app, "message-received", G_CALLBACK (cb_unique_app), NULL);
    }
#endif

  xfconf_channel = xfconf_channel_new_with_property_base ("xfce4-panel", "/plugins/clipman");
  actions = clipman_actions_get ();
  prop_dialog_run ();
  g_object_unref (xfconf_channel);
  g_object_unref (actions);
  xfconf_shutdown ();
  return 0;
}
