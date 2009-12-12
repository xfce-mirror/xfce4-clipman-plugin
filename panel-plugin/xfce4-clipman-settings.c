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

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <libxfcegui4/libxfcegui4.h>
#include <xfconf/xfconf.h>

#include "common.h"
#include "settings-dialog_glade.h"
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
static GladeXML *gxml = NULL;
static ClipmanActions *actions = NULL;
static GtkWidget *settings_dialog = NULL;
static guint test_regex_changed_timeout = 0;



static void
prop_dialog_run ()
{
  GtkWidget *action_dialog;

  /* GladeXML */
  gxml = glade_xml_new_from_buffer (settings_dialog_glade, settings_dialog_glade_length, NULL, NULL);

  /* Dialogs */
  settings_dialog = glade_xml_get_widget (gxml, "settings-dialog");
  action_dialog = glade_xml_get_widget (gxml, "action-dialog");

  /* General settings */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "add-selections")),
                                DEFAULT_ADD_PRIMARY_CLIPBOARD);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "history-ignore-selections")),
                                DEFAULT_HISTORY_IGNORE_PRIMARY_CLIPBOARD);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "save-on-quit")),
                                DEFAULT_SAVE_ON_QUIT);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "store-an-image")),
                                (gboolean)DEFAULT_MAX_IMAGES_IN_HISTORY);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget (gxml, "max-texts-in-history")),
                             (gdouble)DEFAULT_MAX_TEXTS_IN_HISTORY);
  xfconf_g_property_bind (xfconf_channel, "/settings/add-primary-clipboard", G_TYPE_BOOLEAN,
                          G_OBJECT (glade_xml_get_widget (gxml, "add-selections")), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/history-ignore-primary-clipboard", G_TYPE_BOOLEAN,
                          G_OBJECT (glade_xml_get_widget (gxml, "history-ignore-selections")), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          G_OBJECT (glade_xml_get_widget (gxml, "save-on-quit")), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/max-images-in-history", G_TYPE_UINT,
                          G_OBJECT (glade_xml_get_widget (gxml, "store-an-image")), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/max-texts-in-history", G_TYPE_UINT,
                          G_OBJECT (glade_xml_get_widget (gxml, "max-texts-in-history")), "value");

  /* Actions tab and dialog */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "enable-actions")),
                                DEFAULT_ENABLE_ACTIONS);
  xfconf_g_property_bind (xfconf_channel, "/settings/enable-actions", G_TYPE_BOOLEAN,
                          G_OBJECT (glade_xml_get_widget (gxml, "enable-actions")), "active");

  glade_xml_signal_connect_data (gxml, "cb_add_action", G_CALLBACK (cb_add_action), NULL);
  glade_xml_signal_connect_data (gxml, "cb_edit_action", G_CALLBACK (cb_edit_action), NULL);
  glade_xml_signal_connect_data (gxml, "cb_delete_action", G_CALLBACK (cb_delete_action), NULL);
  glade_xml_signal_connect_data (gxml, "cb_reset_actions", G_CALLBACK (cb_reset_actions), NULL);
  glade_xml_signal_connect_data (gxml, "cb_actions_row_activated", G_CALLBACK (cb_actions_row_activated), NULL);
  glade_xml_signal_connect_data (gxml, "cb_add_command", G_CALLBACK (cb_add_command), NULL);
  glade_xml_signal_connect_data (gxml, "cb_refresh_command", G_CALLBACK (cb_refresh_command), NULL);
  glade_xml_signal_connect_data (gxml, "cb_delete_command", G_CALLBACK (cb_delete_command), NULL);
  glade_xml_signal_connect_data (gxml, "cb_show_help", G_CALLBACK (cb_show_help), NULL);
  glade_xml_signal_connect_data (gxml, "cb_test_regex", G_CALLBACK (cb_test_regex), NULL);
  glade_xml_signal_connect_data (gxml, "cb_test_regex_changed", G_CALLBACK (cb_test_regex_changed), NULL);

  setup_actions_treeview (GTK_TREE_VIEW (glade_xml_get_widget (gxml, "actions")));
  setup_commands_treeview (GTK_TREE_VIEW (glade_xml_get_widget (gxml, "commands")));
  setup_test_regex_dialog ();

  /* Callbacks for the OK button sensitivity in the edit action dialog */
  g_signal_connect_after (glade_xml_get_widget (gxml, "action-name"), "changed",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (glade_xml_get_widget (gxml, "regex"), "changed",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (glade_xml_get_widget (gxml, "button-add-command"), "clicked",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (glade_xml_get_widget (gxml, "button-delete-command"), "clicked",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);

  /* Run the dialog */
  while ((gtk_dialog_run (GTK_DIALOG (settings_dialog))) == 2);

  gtk_widget_destroy (action_dialog);
  gtk_widget_destroy (settings_dialog);
  g_object_unref (gxml);

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

  xfce_err ("Unable to open documentation \"%s\"", filename);

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
                                       entry->action_name, g_regex_get_pattern (entry->regex));
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

  action_name = gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (gxml, "action-name")));
  regex = gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (gxml, "regex")));
  group = (gint)gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "manual")));

  treeview = glade_xml_get_widget (gxml, "commands");
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
  treeview = glade_xml_get_widget (gxml, "actions");
  refresh_actions_treeview (GTK_TREE_VIEW (treeview));
}

static void
cb_actions_selection_changed (GtkTreeSelection *selection)
{
  GtkTreeModel *model;
  gboolean sensitive;

  sensitive = gtk_tree_selection_get_selected (selection, &model, NULL);

  gtk_widget_set_sensitive (glade_xml_get_widget (gxml, "button-edit-action"), sensitive);
  gtk_widget_set_sensitive (glade_xml_get_widget (gxml, "button-delete-action"), sensitive);
}

static void
cb_add_action (GtkButton *button)
{
  GtkWidget *action_dialog;
  gint res;

  action_dialog = glade_xml_get_widget (gxml, "action-dialog");
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

  treeview = glade_xml_get_widget (gxml, "actions");

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

  action_dialog = glade_xml_get_widget (gxml, "action-dialog");
  entry_dialog_cleanup ();

  actions_model = gtk_tree_view_get_model (treeview);
  gtk_tree_model_get_iter (actions_model, &iter, path);
  gtk_tree_model_get (actions_model, &iter, 0, &entry, -1);

  commands_model = gtk_tree_view_get_model (GTK_TREE_VIEW (glade_xml_get_widget (gxml, "commands")));
#if GLIB_CHECK_VERSION (2,16,0)
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
#else
  g_hash_table_foreach (entry->commands, (GHFunc)__foreach_command_fill_commands, commands_model);
#endif

  gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gxml, "action-name")), entry->action_name);
  gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gxml, "regex")), g_regex_get_pattern (entry->regex));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "manual")), entry->group);

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

  treeview = glade_xml_get_widget (gxml, "actions");

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
  refresh_actions_treeview (GTK_TREE_VIEW (glade_xml_get_widget (gxml, "actions")));
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
entry_dialog_cleanup ()
{
  GtkTreeModel *model;

  gtk_widget_set_sensitive (glade_xml_get_widget (gxml, "action-dialog-button-ok"), FALSE);

  gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gxml, "action-name")), "");
  gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gxml, "regex")), "");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "manual")), FALSE);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (glade_xml_get_widget (gxml, "commands")));
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

  gtk_widget_set_sensitive (glade_xml_get_widget (gxml, "button-refresh-command"), sensitive);
  gtk_widget_set_sensitive (glade_xml_get_widget (gxml, "button-delete-command"), sensitive);

  if (sensitive)
    {
      gtk_tree_model_get (model, &iter, 1, &command_name, 2, &command, -1);
      gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gxml, "command-name")), command_name);
      gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gxml, "command")), command);
      g_free (command_name);
      g_free (command);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gxml, "command-name")), "");
      gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gxml, "command")), "");
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

  command_name = glade_xml_get_widget (gxml, "command-name");
  command = glade_xml_get_widget (gxml, "command");

  if (gtk_entry_get_text (GTK_ENTRY (command_name))[0] == '\0'
      || gtk_entry_get_text (GTK_ENTRY (command))[0] == '\0')
    return;

  title = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>",
                                   gtk_entry_get_text (GTK_ENTRY (command_name)),
                                   gtk_entry_get_text (GTK_ENTRY (command)));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (glade_xml_get_widget (gxml, "commands")));
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

  command_name = glade_xml_get_widget (gxml, "command-name");
  command = glade_xml_get_widget (gxml, "command");

  if (gtk_entry_get_text (GTK_ENTRY (command_name))[0] == '\0'
      || gtk_entry_get_text (GTK_ENTRY (command))[0] == '\0')
    return;

  treeview = glade_xml_get_widget (gxml, "commands");
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

  treeview = glade_xml_get_widget (gxml, "commands");

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
setup_test_regex_dialog ()
{
  GtkWidget *textview;
  GtkTextBuffer *buffer;

  textview = glade_xml_get_widget (gxml, "regex-textview");
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
  GtkWidget *textview;
  const gchar *pattern;

  regex = glade_xml_get_widget (gxml, "regex");
  dialog = glade_xml_get_widget (gxml, "regex-dialog");
  entry = glade_xml_get_widget (gxml, "regex-entry");

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
  if (test_regex_changed_timeout > 0)
    g_source_remove (test_regex_changed_timeout);

  test_regex_changed_timeout = g_timeout_add_seconds (1, (GSourceFunc)cb_test_regex_changed_timeout, NULL);
}

static gboolean
cb_test_regex_changed_timeout ()
{
  update_test_regex_textview_tags ();
  test_regex_changed_timeout = 0;
  return FALSE;
}

static void
update_test_regex_textview_tags ()
{
  GtkWidget *entry;
  GtkWidget *textview;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  GRegex *regex;
  GMatchInfo *match_info = NULL;
  const gchar *pattern;
  gchar *text;

  entry = glade_xml_get_widget (gxml, "regex-entry");
  textview = glade_xml_get_widget (gxml, "regex-textview");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
  gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
  gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);

  /* Remove all tags */
  gtk_text_buffer_remove_all_tags (buffer, &start, &end);

  /* Build Regex */
  pattern = gtk_entry_get_text (GTK_ENTRY (entry));
  regex = g_regex_new (pattern, G_REGEX_DOTALL|G_REGEX_CASELESS, 0, NULL);
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

  action_name = gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (gxml, "action-name")));
  regex_pattern = gtk_entry_get_text (GTK_ENTRY (glade_xml_get_widget (gxml, "regex")));
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (glade_xml_get_widget (gxml, "commands")));
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

  gtk_widget_set_sensitive (glade_xml_get_widget (gxml, "action-dialog-button-ok"), sensitive);
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
  UniqueApp *app = unique_app_new ("org.xfce.Clipman", NULL);
  if (unique_app_is_running (app))
    {
      if (unique_app_send_message (app, UNIQUE_ACTIVATE, NULL) == UNIQUE_RESPONSE_OK)
        {
          g_object_unref (app);
          return;
        }
    }
  g_signal_connect (app, "message-received", G_CALLBACK (cb_unique_app), NULL);
#endif

  xfconf_channel = xfconf_channel_new_with_property_base ("xfce4-panel", "/plugins/clipman");
  actions = clipman_actions_get ();
  prop_dialog_run ();
  g_object_unref (xfconf_channel);
  g_object_unref (actions);
  xfconf_shutdown ();
  return 0;
}

