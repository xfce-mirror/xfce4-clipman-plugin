/*
 *  Copyright (c) 2009-2012 Mike Massonnet <mmassonnet@xfce.org>
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

#include "actions.h"
#include "common.h"

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

static void
cb_show_help (GtkButton *button);
static void
setup_actions_treeview (GtkTreeView *treeview);
static void
refresh_actions_treeview (GtkTreeView *treeview);
static void
apply_action (const gchar *original_action_name);
static void
cb_actions_selection_changed (GtkTreeSelection *selection);
static void
cb_add_action (GtkButton *button);
static void
cb_edit_action (GtkButton *button);
static void
cb_actions_row_activated (GtkTreeView *treeview,
                          GtkTreePath *path,
                          GtkTreeViewColumn *column);
static void
cb_delete_action (GtkButton *button);
static void
cb_reset_actions (GtkButton *button);
static void
setup_commands_treeview (GtkTreeView *treeview);
static void
entry_dialog_cleanup (void);
static void
cb_commands_selection_changed (GtkTreeSelection *selection);
static void
cb_add_command (GtkButton *button);
static void
cb_refresh_command (GtkButton *button);
static void
cb_delete_command (GtkButton *button);
static void
setup_test_regex_dialog (void);
static void
cb_test_regex (GtkButton *button);
static void
cb_test_regex_changed (GtkWidget *widget);
static gboolean
cb_regex_focus_in_event (GtkWidget *widget,
                         GdkEvent *event,
                         gpointer user_data);
static gboolean
cb_regex_focus_out_event (GtkWidget *widget,
                          GdkEvent *event,
                          gpointer user_data);
static void
update_test_regex_textview_tags (void);
static void
cb_set_action_dialog_button_ok (GtkWidget *widget);

static XfconfChannel *xfconf_channel = NULL;
static GtkBuilder *builder = NULL;
static ClipmanActions *actions = NULL;
static GtkWidget *settings_dialog = NULL;
static guint test_regex_changed_timeout = 0;



static void
prop_dialog_init (void)
{
  GtkWidget *action_dialog;
  GtkWidget *combobox;

  /* Dialogs */
  action_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "action-dialog"));

  /* Behavior tab: General */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "add-selections")),
                                DEFAULT_ADD_PRIMARY_CLIPBOARD);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "persistent-selections")),
                                DEFAULT_PERSISTENT_PRIMARY_CLIPBOARD);
#ifdef HAVE_QRENCODE
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "show-qr-code")),
                                DEFAULT_SHOW_QR_CODE);
#else
  gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "show-qr-code")));
#endif
  if (!WINDOWING_IS_X11 ())
    gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "persistent-selections")));

  /* paste-on-activate combobox */
  combobox = GTK_WIDGET (gtk_builder_get_object (builder, "combobox-paste-on-activate"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), _("None"));
  /* TRANSLATORS: Keyboard shortcut */
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), _("Ctrl+V"));
  /* TRANSLATORS: Keyboard shortcut */
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), _("Shift+Insert"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);
  if (!WINDOWING_IS_X11 ())
    gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "hbox1")));

#ifdef HAVE_QRENCODE
  xfconf_g_property_bind (xfconf_channel, "/settings/show-qr-code", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "show-qr-code"), "active");
#endif
  xfconf_g_property_bind (xfconf_channel, "/settings/add-primary-clipboard", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "add-selections"), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/persistent-primary-clipboard", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "persistent-selections"), "active");
  xfconf_g_property_bind (xfconf_channel, "/tweaks/paste-on-activate",
                          G_TYPE_UINT, G_OBJECT (combobox), "active");

  /* Behavior tab: Menu */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "popup-at-pointer")),
                                DEFAULT_POPUP_AT_POINTER);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "max-menu-items")),
                             (gdouble) DEFAULT_MAX_MENU_ITEMS);

  xfconf_g_property_bind (xfconf_channel, "/tweaks/popup-at-pointer", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "popup-at-pointer"), "active");
  xfconf_g_property_bind (xfconf_channel, "/tweaks/max-menu-items", G_TYPE_UINT,
                          gtk_builder_get_object (builder, "max-menu-items"), "value");

  /* Actions tab and dialog */
  gtk_switch_set_state (GTK_SWITCH (gtk_builder_get_object (builder, "enable-actions")),
                        DEFAULT_ENABLE_ACTIONS);
  xfconf_g_property_bind (xfconf_channel, "/settings/enable-actions", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "enable-actions"), "active");

  combobox = GTK_WIDGET (gtk_builder_get_object (builder, "combobox-skip-action"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), _("shows actions"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), _("hides actions"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);
  if (!WINDOWING_IS_X11 ())
    gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "hbox-skip-action")));

  xfconf_g_property_bind (xfconf_channel, "/settings/enable-actions", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "box-actions"), "sensitive");
  xfconf_g_property_bind (xfconf_channel, "/tweaks/skip-action-on-key-down",
                          G_TYPE_BOOLEAN, G_OBJECT (combobox), "active");

  g_signal_connect (gtk_builder_get_object (builder, "button-add-action"), "clicked", G_CALLBACK (cb_add_action), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-edit-action"), "clicked", G_CALLBACK (cb_edit_action), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-delete-action"), "clicked", G_CALLBACK (cb_delete_action), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-reset-actions"), "clicked", G_CALLBACK (cb_reset_actions), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "actions"), "row_activated", G_CALLBACK (cb_actions_row_activated), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-add-command"), "clicked", G_CALLBACK (cb_add_command), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-refresh-command"), "clicked", G_CALLBACK (cb_refresh_command), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-delete-command"), "clicked", G_CALLBACK (cb_delete_command), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "settings-dialog-button-help"), "clicked", G_CALLBACK (cb_show_help), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "button-action-pattern"), "clicked", G_CALLBACK (cb_test_regex), NULL);
  g_signal_connect (gtk_builder_get_object (builder, "regex-entry"), "changed", G_CALLBACK (cb_test_regex_changed), NULL);

  g_signal_connect_swapped (gtk_builder_get_object (builder, "settings-dialog-button-close"), "clicked",
                            G_CALLBACK (gtk_widget_destroy), settings_dialog);

  setup_actions_treeview (GTK_TREE_VIEW (gtk_builder_get_object (builder, "actions")));
  setup_commands_treeview (GTK_TREE_VIEW (gtk_builder_get_object (builder, "commands")));
  setup_test_regex_dialog ();

  /* Callbacks for the OK button sensitivity in the edit action dialog */
  g_signal_connect_after (gtk_builder_get_object (builder, "action-name"), "changed",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (gtk_builder_get_object (builder, "regex"), "changed",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (gtk_builder_get_object (builder, "regex"), "focus-in-event",
                          G_CALLBACK (cb_regex_focus_in_event), gtk_builder_get_object (builder, "regex-infobar"));
  g_signal_connect_after (gtk_builder_get_object (builder, "regex"), "focus-out-event",
                          G_CALLBACK (cb_regex_focus_out_event), gtk_builder_get_object (builder, "regex-infobar"));
  g_signal_connect_after (gtk_builder_get_object (builder, "button-add-command"), "clicked",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);
  g_signal_connect_after (gtk_builder_get_object (builder, "button-delete-command"), "clicked",
                          G_CALLBACK (cb_set_action_dialog_button_ok), NULL);

  g_signal_connect_swapped (gtk_builder_get_object (builder, "action-dialog-button-cancel"), "clicked",
                            G_CALLBACK (gtk_dialog_response), action_dialog);

  /* History tab */
  gtk_switch_set_state (GTK_SWITCH (gtk_builder_get_object (builder, "save-on-quit")),
                        DEFAULT_SAVE_ON_QUIT);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "store-an-image")),
                                (gboolean) DEFAULT_MAX_IMAGES_IN_HISTORY);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "reorder-items")),
                                DEFAULT_REORDER_ITEMS);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "reverse-order")),
                                DEFAULT_REVERSE_ORDER);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "history-ignore-selections")),
                                DEFAULT_HISTORY_IGNORE_PRIMARY_CLIPBOARD);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gtk_builder_get_object (builder, "max-texts-in-history")),
                             (gdouble) DEFAULT_MAX_TEXTS_IN_HISTORY);

  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "save-on-quit"), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/max-images-in-history", G_TYPE_UINT,
                          gtk_builder_get_object (builder, "store-an-image"), "active");
  xfconf_g_property_bind (xfconf_channel, "/tweaks/reorder-items", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "reorder-items"), "active");
  xfconf_g_property_bind (xfconf_channel, "/tweaks/reverse-menu-order", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "reverse-order"), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/history-ignore-primary-clipboard", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "history-ignore-selections"), "active");
  xfconf_g_property_bind (xfconf_channel, "/settings/max-texts-in-history", G_TYPE_UINT,
                          gtk_builder_get_object (builder, "max-texts-in-history"), "value");
  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "store-an-image"), "sensitive");
  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "reorder-items"), "sensitive");
  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "reverse-order"), "sensitive");
  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "history-ignore-selections"), "sensitive");
  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "max-texts-in-history"), "sensitive");
  xfconf_g_property_bind (xfconf_channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          gtk_builder_get_object (builder, "label-history-size"), "sensitive");
}

static void
cb_show_help (GtkButton *button)
{
  gchar *docpath = NULL;
  gchar *command = NULL;
  gchar *locale = NULL;
  gchar *offset;

  /* Find localized documentation path on disk */
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

  docpath = g_strdup_printf (DOCDIR "/html/%s/index.html", locale);
  if (!g_file_test (docpath, G_FILE_TEST_EXISTS))
    {
      offset = g_strrstr (locale, "_");
      if (offset == NULL)
        {
          g_free (docpath);
          docpath = g_strdup (DOCDIR "/html/C/index.html");
        }
      else
        {
          *offset = '\0';
          g_free (docpath);
          docpath = g_strdup_printf (DOCDIR "/html/%s/index.html", locale);
          if (!g_file_test (docpath, G_FILE_TEST_EXISTS))
            {
              g_free (docpath);
              docpath = g_strdup (DOCDIR "/html/C/index.html");
            }
        }
    }

  g_free (locale);

  /* Revert to online documentation if not available on disk */
  if (g_file_test (docpath, G_FILE_TEST_EXISTS))
    {
      gchar *tmp = docpath;
      docpath = g_strdup_printf ("file://%s", docpath);
      g_free (tmp);
    }
  else
    {
      g_free (docpath);
      docpath = g_strdup (PACKAGE_URL);
    }

  /* Open documentation in webbrowser */
  command = g_strdup_printf ("exo-open --launch WebBrowser %s", docpath);
  if (g_spawn_command_line_async (command, NULL))
    goto out;

  g_free (command);
  command = g_strdup_printf ("firefox %s", docpath);
  if (g_spawn_command_line_async (command, NULL))
    goto out;

  xfce_dialog_show_error (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
                          NULL, "Unable to open documentation \"%s\"", docpath);

out:
  g_free (docpath);
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
  group = (gint) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "manual")));

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

  {
    GHashTableIter hiter;
    gpointer key, value;
    g_hash_table_iter_init (&hiter, entry->commands);
    while (g_hash_table_iter_next (&hiter, &key, &value))
      {
        title = g_markup_printf_escaped ("<b>%s</b>\n<small>%s</small>", (gchar *) key, (gchar *) value);
        gtk_list_store_append (GTK_LIST_STORE (commands_model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (commands_model), &iter, 0, title, 1, key, 2, value, -1);
        g_free (title);
      }
  }

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
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_YES_NO,
                                               _("<b>Reset actions</b>"));
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

static gboolean
cb_test_regex_changed_timeout (gpointer user_data)
{
  update_test_regex_textview_tags ();
  test_regex_changed_timeout = 0;
  return FALSE;
}

static void
cb_test_regex_changed (GtkWidget *widget)
{
  if (test_regex_changed_timeout == 0)
    gtk_entry_set_icon_from_icon_name (GTK_ENTRY (gtk_builder_get_object (builder, "regex-entry")),
                                       GTK_ENTRY_ICON_SECONDARY, "gtk-refresh");

  if (test_regex_changed_timeout > 0)
    g_source_remove (test_regex_changed_timeout);

  test_regex_changed_timeout = g_timeout_add_seconds (1, cb_test_regex_changed_timeout, NULL);
}

static gboolean
cb_regex_focus_in_event (GtkWidget *widget,
                         GdkEvent *event,
                         gpointer user_data)
{
  GtkWidget *infobar = GTK_WIDGET (user_data);

  gtk_widget_show (infobar);
  gtk_info_bar_set_revealed (GTK_INFO_BAR (infobar), TRUE);

  return FALSE;
}

static gboolean
cb_regex_focus_out_event (GtkWidget *widget,
                          GdkEvent *event,
                          gpointer user_data)
{
  GtkWidget *infobar = GTK_WIDGET (user_data);

  gtk_info_bar_set_revealed (GTK_INFO_BAR (infobar), FALSE);
  gtk_widget_hide (infobar);

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
  regex = g_regex_new (pattern, G_REGEX_CASELESS | G_REGEX_MULTILINE, 0, NULL);
  if (regex == NULL)
    {
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, "dialog-error");
      return;
    }
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, "dialog-apply");

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
  while (g_match_info_next (match_info, NULL));

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



static void
shutdown (GApplication *app,
          gpointer user_data)
{
  clipman_actions_save (actions);
  g_object_unref (actions);
  g_object_unref (builder);
  g_object_unref (xfconf_channel);
  xfconf_shutdown ();
}



static gint
command_line (GApplication *app,
              GApplicationCommandLine *command_line,
              gpointer user_data)
{
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

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_resource (builder, "/org/xfce/clipman-plugin/settings-dialog.ui", &error))
    {
      g_critical ("GtkBuilder loading failed: %s", error->message);
      g_error_free (error);
      g_object_unref (builder);
      return EXIT_FAILURE;
    }

  settings_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "settings-dialog"));
  g_signal_connect_swapped (app, "activate", G_CALLBACK (gtk_window_present), settings_dialog);
  g_signal_connect (app, "shutdown", G_CALLBACK (shutdown), NULL);
  xfconf_channel = xfconf_channel_new_with_property_base ("xfce4-panel", "/plugins/clipman");
  actions = clipman_actions_get ();
  prop_dialog_init ();
  gtk_window_set_application (GTK_WINDOW (settings_dialog), GTK_APPLICATION (app));
  gtk_widget_show (settings_dialog);

  return EXIT_SUCCESS;
}



gint
main (gint argc,
      gchar *argv[])
{
  GtkApplication *app;
  gint ret;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  app = gtk_application_new ("org.xfce.clipman.settings", G_APPLICATION_HANDLES_COMMAND_LINE);
  g_signal_connect (app, "command-line", G_CALLBACK (command_line), NULL);
  ret = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return ret;
}
