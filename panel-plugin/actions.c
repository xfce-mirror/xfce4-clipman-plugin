/*
 *  Copyright (c) 2009-2012 Mike Massonnet <mmassonnet@xfce.org>
 *
 *  XML parsing based on Xfce4 Panel:
 *  Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  Internationalization of the XML file based on Thunar User Custom Actions:
 *  Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>

#include "common.h"

#include "actions.h"

/*
 * GObject declarations
 */

struct _ClipmanActionsPrivate
{
  GFile                *file;
  GFileMonitor         *file_monitor;
  GSList               *entries;
  GtkWidget            *menu;
  gboolean              skip_action_on_key_down;
};

G_DEFINE_TYPE_WITH_PRIVATE (ClipmanActions, clipman_actions, G_TYPE_OBJECT)

enum
{
  SKIP_ACTION_ON_KEY_DOWN = 1,
};

static void             clipman_actions_finalize            (GObject *object);
static void             clipman_actions_set_property        (GObject *object,
                                                             guint property_id,
                                                             const GValue *value,
                                                             GParamSpec *pspec);
static void             clipman_actions_get_property        (GObject *object,
                                                             guint property_id,
                                                             GValue *value,
                                                             GParamSpec *pspec);

/*
 * Misc functions declarations
 */

static void            _clipman_actions_free_list           (ClipmanActions *actions);
static gint           __clipman_actions_entry_compare       (gpointer a,
                                                             gpointer b);
static gint           __clipman_actions_entry_compare_name  (gpointer a,
                                                             gpointer b);
static void           __clipman_actions_entry_free          (ClipmanActionsEntry *entry);

/*
 * Callbacks declarations
 */

static void             cb_entry_activated                  (GtkMenuItem *mi,
                                                             gpointer user_data);
static void             cb_file_changed                     (ClipmanActions *actions,
                                                             GFile *file,
                                                             GFile *other_file,
                                                             GFileMonitorEvent event_type);
static gboolean         timeout_file_changed                (ClipmanActions *actions);

/*
 * XML Parser declarations
 */

static void             start_element_handler               (GMarkupParseContext *context,
                                                             const gchar *element_name,
                                                             const gchar **attribute_names,
                                                             const gchar **attribute_values,
                                                             gpointer user_data,
                                                             GError **error);
static void             end_element_handler                 (GMarkupParseContext *context,
                                                             const gchar *element_name,
                                                             gpointer user_data,
                                                             GError **error);
static void             text_handler                        (GMarkupParseContext *context,
                                                             const gchar *text,
                                                             gsize text_len,
                                                             gpointer user_data,
                                                             GError **error);

static GMarkupParser markup_parser =
{
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  NULL,
};

typedef enum
{
  START,
  ACTIONS,
  ACTION,
  ACTION_NAME,
  REGEX,
  GROUP,
  COMMANDS,
  COMMAND,
  COMMAND_NAME,
  EXEC,
} ParserState;

typedef struct _EntryParser EntryParser;
struct _EntryParser
{
  ClipmanActions *actions;
  ParserState state;

  gchar *locale;
  gboolean name_use;
  gint name_match;

  gchar *action_name;
  gchar *regex;
  gint group;
  gchar *command_name;
  gchar *command;
};



/*
 * XML Parser
 */

static void
start_element_handler (GMarkupParseContext *context,
                       const gchar *element_name,
                       const gchar **attribute_names,
                       const gchar **attribute_values,
                       gpointer user_data,
                       GError **error)
{
  EntryParser *parser = user_data;
  gint n;
  gint match;

  switch (parser->state)
    {
    case START:
      if (!g_ascii_strcasecmp (element_name, "actions"))
        parser->state = ACTIONS;
      break;

    case ACTIONS:
      parser->name_use = FALSE;
      parser->name_match = XFCE_LOCALE_NO_MATCH;

      if (!g_ascii_strcasecmp (element_name, "action"))
        parser->state = ACTION;
      break;

    case COMMANDS:
      parser->name_use = FALSE;
      parser->name_match = XFCE_LOCALE_NO_MATCH;

      if (!g_ascii_strcasecmp (element_name, "command"))
        parser->state = COMMAND;
      break;

    case ACTION:
    case COMMAND:
      if (!g_ascii_strcasecmp (element_name, "name"))
        {
          for (n = 0; attribute_names[n] != NULL; n++)
            {
              if (!g_ascii_strcasecmp (attribute_names[n], "xml:lang"))
                break;
            }

          if (attribute_names[n] == NULL)
            {
              parser->name_use = (parser->name_match <= XFCE_LOCALE_NO_MATCH);
            }
          else
            {
              match = xfce_locale_match (parser->locale, attribute_values[n]);
              if (parser->name_match < match)
                {
                  parser->name_match = match;
                  parser->name_use = TRUE;
                }
              else
                parser->name_use = FALSE;
            }

          parser->state = (parser->state == ACTION) ? ACTION_NAME : COMMAND_NAME;
        }
      else if (!g_ascii_strcasecmp (element_name, "regex"))
        parser->state = REGEX;
      else if (!g_ascii_strcasecmp (element_name, "group"))
        parser->state = GROUP;
      else if (!g_ascii_strcasecmp (element_name, "commands"))
        parser->state = COMMANDS;
      else if (!g_ascii_strcasecmp (element_name, "exec"))
        parser->state = EXEC;
      break;

    default:
      break;
    }
}

static void
end_element_handler (GMarkupParseContext *context,
                     const gchar *element_name,
                     gpointer user_data,
                     GError **error)
{
  EntryParser *parser = user_data;

  switch (parser->state)
    {
    case ACTION:
      g_free (parser->action_name);
      g_free (parser->regex);
      parser->action_name = NULL;
      parser->regex = NULL;
      parser->group = 0;

      parser->state = ACTIONS;
      break;

    case ACTION_NAME:
    case REGEX:
    case GROUP:
    case COMMANDS:
      parser->state = ACTION;
      break;

    case COMMAND:
      if (parser->action_name == NULL || parser->regex == NULL)
        {
          g_warning ("Closing a command but no action name nor regex set");
        }
      else
        {
          clipman_actions_add (parser->actions, parser->action_name, parser->regex,
                               parser->command_name, parser->command);
          clipman_actions_set_group (parser->actions, parser->action_name, parser->group);
        }

      g_free (parser->command_name);
      g_free (parser->command);
      parser->command_name = NULL;
      parser->command = NULL;

      parser->state = COMMANDS;
      break;

    case COMMAND_NAME:
    case EXEC:
      parser->state = COMMAND;
      break;

    default:
      break;
    }
}

static void
text_handler (GMarkupParseContext *context,
              const gchar *text,
              gsize text_len,
              gpointer user_data,
              GError **error)
{
  EntryParser *parser = user_data;

  switch (parser->state)
    {
    case ACTION_NAME:
      if (parser->name_use)
        {
          g_free (parser->action_name);
          parser->action_name = g_strdup (text);
        }
      break;

    case REGEX:
      parser->regex = g_strdup (text);
      break;

    case GROUP:
      parser->group = (gint)g_strtod (text, NULL);
      break;

    case COMMAND_NAME:
      if (parser->name_use)
        {
          g_free (parser->command_name);
          parser->command_name = g_strdup (text);
        }
      break;

    case EXEC:
      parser->command = g_strdup (text);
      break;

    default:
      break;
    }
}

/*
 * Callbacks
 */

static void
cb_entry_activated (GtkMenuItem *mi,
                    gpointer user_data)
{
  gchar *real_command;
  const gchar *text;
  const gchar *command;
  const GRegex *regex;
  GError *error = NULL;

  text = g_object_get_data (G_OBJECT (mi), "text");
  command = g_object_get_data (G_OBJECT (mi), "command");
  regex = g_object_get_data (G_OBJECT (mi), "regex");

  real_command = g_regex_replace (regex, text, -1, 0, command, 0, NULL);

  DBG ("Execute command `%s'", real_command);

  g_spawn_command_line_async (real_command, &error);
  if (error != NULL)
    {
      xfce_dialog_show_error (NULL, error, _("Unable to execute the command \"%s\"\n\n%s"), real_command, error->message);
      g_error_free (error);
    }
  g_free (real_command);
}

static void
cb_file_changed (ClipmanActions *actions,
                 GFile *file,
                 GFile *other_file,
                 GFileMonitorEvent event_type)
{
  static GSource *source = NULL;
  guint           source_id;

  if (event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
    {
      /* drop the previous timer source */
      if (source != NULL)
        {
          if (! g_source_is_destroyed (source))
            g_source_destroy (source);

          g_source_unref (source);
          source = NULL;
        }

      source_id = g_timeout_add_seconds (1, (GSourceFunc)timeout_file_changed, actions);

      /* retrieve the timer source and increase its ref count to test its destruction next time */
      source = g_main_context_find_source_by_id (NULL, source_id);
      g_source_ref (source);
    }
}

static gboolean
timeout_file_changed (ClipmanActions *actions)
{
  _clipman_actions_free_list (actions);
  clipman_actions_load (actions);
  return FALSE;
}

/*
 * Misc functions
 */

static void
_clipman_actions_free_list (ClipmanActions *actions)
{
  GSList *l;
  for (l = actions->priv->entries; l != NULL; l = l->next)
    __clipman_actions_entry_free (l->data);
  g_slist_free (actions->priv->entries);
  actions->priv->entries = NULL;
}

static gint
__clipman_actions_entry_compare (gpointer a,
                                 gpointer b)
{
  const ClipmanActionsEntry *entrya = a;
  const ClipmanActionsEntry *entryb = b;
  return g_ascii_strcasecmp (entrya->action_name, entryb->action_name);
}

static gint
__clipman_actions_entry_compare_name (gpointer a,
                                      gpointer b)
{
  const ClipmanActionsEntry *entry = a;
  const char *name = b;
  return g_ascii_strcasecmp (entry->action_name, name);
}

static void
__clipman_actions_entry_free (ClipmanActionsEntry *entry)
{
  g_free (entry->action_name);
  g_free (entry->pattern);
  g_regex_unref (entry->regex);
  g_hash_table_destroy (entry->commands);
  g_slice_free (ClipmanActionsEntry, entry);
}

/*
 * Public methods
 */

/**
 * clipman_actions_add:
 * @actions:        a #ClipmanActions
 * @action_name:    human readable name of @regex
 * @regex:          a valid regex or NULL if @name already exists in the list
 * @command_name:   human readable name of @command
 * @command:        the command to execute
 *
 * Adds a new entry to the list of actions.  The same name and regex can be
 * passed several times for each new command.
 * If @action_name already exists, the regex is ignored.
 * If @command_name already exists, the new command will replace the old one.
 *
 * The command can contain the parameter '%s' that will be replaced by the
 * matching regex text.
 *
 * The action is created with the default group 0. To change it use the
 * function clipman_actions_set_group().
 *
 * Returns: FALSE if the regex was invalid
 */
gboolean
clipman_actions_add (ClipmanActions *actions,
                     const gchar *action_name,
                     const gchar *regex,
                     const gchar *command_name,
                     const gchar *command)
{
  ClipmanActionsEntry *entry;
  GSList *l;
  GRegex *_regex;
  gchar *regex_anchored;

  g_return_val_if_fail (G_LIKELY (action_name != NULL), FALSE);
  g_return_val_if_fail (G_LIKELY (command_name != NULL), FALSE);
  g_return_val_if_fail (G_LIKELY (command != NULL), FALSE);

  l = g_slist_find_custom (actions->priv->entries, action_name, (GCompareFunc)__clipman_actions_entry_compare_name);

  /* Add a new entry to the list */
  if (l == NULL)
    {
      /* Validate the regex */
      regex_anchored = g_strdup_printf ("%s$", regex);
      _regex = g_regex_new (regex_anchored, G_REGEX_CASELESS|G_REGEX_ANCHORED, 0, NULL);
      g_free (regex_anchored);
      if (_regex == NULL)
        return FALSE;

      DBG ("New entry `%s' with command `%s'", action_name, command_name);

      entry = g_slice_new0 (ClipmanActionsEntry);
      entry->action_name = g_strdup (action_name);
      entry->pattern = g_strdup (regex);
      entry->regex = _regex;
      entry->group = 0;
      entry->commands = g_hash_table_new_full ((GHashFunc)g_str_hash, (GEqualFunc)g_str_equal,
                                               (GDestroyNotify)g_free, (GDestroyNotify)g_free);
      g_hash_table_insert (entry->commands, g_strdup (command_name), g_strdup (command));

      actions->priv->entries = g_slist_insert_sorted (actions->priv->entries, entry, (GCompareFunc)__clipman_actions_entry_compare);
      return TRUE;
    }

  /* Add command to the existing entry */
  DBG ("Add to entry `%s' the command `%s'", action_name, command_name);

  entry = l->data;
  g_hash_table_insert (entry->commands, g_strdup (command_name), g_strdup (command));
  return TRUE;
}

/**
 * clipman_actions_remove:
 * @actions:        a #ClipmanActions
 * @action_name:    the human readable name for the regex
 *
 * Removes a #ClipmanActionsEntry from the list of actions.
 *
 * Returns: FALSE if no action could be removed
 */
gboolean
clipman_actions_remove (ClipmanActions *actions,
                        const gchar *action_name)
{
  ClipmanActionsEntry *entry;
  GSList *l;

  l = g_slist_find_custom (actions->priv->entries, action_name, (GCompareFunc)__clipman_actions_entry_compare_name);
  if (l == NULL)
    {
      g_warning ("No corresponding entry `%s'", action_name);
      return FALSE;
    }

  DBG ("Drop the entry `%s'", action_name);

  entry = l->data;
  __clipman_actions_entry_free (entry);
  actions->priv->entries = g_slist_delete_link (actions->priv->entries, l);

  return TRUE;
}

/**
 * clipman_actions_remove_command:
 * @actions:        a #ClipmanActions
 * @action_name:    the human readable name for the regex
 * @command_name:   the command to remove
 *
 * Removes a command from the list of actions.  If the command is the last
 * command for the corresponding action, than the action will be completely
 * dropped from the list.
 *
 * Returns: FALSE if no command could be removed
 */
gboolean
clipman_actions_remove_command (ClipmanActions *actions,
                                const gchar *action_name,
                                const gchar *command_name)
{
  ClipmanActionsEntry *entry;
  GSList *l;
  gboolean found;

  l = g_slist_find_custom (actions->priv->entries, action_name, (GCompareFunc)__clipman_actions_entry_compare_name);
  if (l == NULL)
    {
      g_warning ("No corresponding entry `%s'", action_name);
      return FALSE;
    }

  entry = l->data;
  found = g_hash_table_remove (entry->commands, command_name);

  if (!found)
    g_warning ("No corresponding command `%s' inside entry `%s'", command_name, action_name);
  else
    {
      DBG ("Drop from entry `%s' the command `%s'", action_name, command_name);
      if (g_hash_table_size (entry->commands) == 0)
        {
          DBG ("Clean up the entry");
          __clipman_actions_entry_free (entry);
          actions->priv->entries = g_slist_delete_link (actions->priv->entries, l);
        }
    }

  return found;
}

/**
 * clipman_actions_set_group:
 * @actions:            a #ClipmanActions
 * @action_name:        the human readable name for the regex
 * @group:              the group identifier
 *
 * Changes the group of @action_name to @group.
 */
void
clipman_actions_set_group (ClipmanActions *actions,
                           const gchar *action_name,
                           gint group)
{
  ClipmanActionsEntry *entry;
  GSList *l;

  l = g_slist_find_custom (actions->priv->entries, action_name, (GCompareFunc)__clipman_actions_entry_compare_name);
  if (l == NULL)
    {
      g_warning ("No corresponding entry `%s'", action_name);
      return;
    }

  entry = l->data;
  entry->group = group;
}

/**
 * clipman_actions_get_entries:
 * @actions:    a #ClipmanActions
 *
 * Returns: a #const #GSList owned by #ClipmanActions
 */
const GSList *
clipman_actions_get_entries (ClipmanActions *actions)
{
  return actions->priv->entries;
}

/**
 * clipman_actions_match:
 * @actions:    a #ClipmanActions
 * @group:      the group identifier
 * @text:       the text to match against the existing regex's
 *
 * Searches a regex match for @text and returns a newly allocated #GSList which
 * must be freed with g_slist_free() that contains a list of
 * #ClipmanActionsEntry for each matched action.
 * Note that the data inside the list is owned by #ClipmanActions and must not
 * be modified.
 * The @group identifier can be -1 to get a match from all groups.
 *
 * Returns: a newly allocated #GSList
 */
GSList *
clipman_actions_match (ClipmanActions *actions,
                       gint group,
                       const gchar *text)
{
  ClipmanActionsEntry *entry;
  GSList *l;
  GSList *entries = NULL;

  for (l = actions->priv->entries; l != NULL; l = l->next)
    {
      entry = l->data;
      if (group == -1 || group == entry->group)
        {
          if (g_regex_match (entry->regex, text, 0, NULL))
            entries = g_slist_prepend (entries, entry);
        }
    }

  return entries;
}

/**
 * clipman_actions_match_with_menu:
 * @actions:    a #ClipmanActions
 * @group:      the group identifier
 * @text:       the text to match against the existing regex's
 *
 * Builds and displays a menu with matching actions for @text.
 *
 * Read clipman_actions_match() for more information.
 */
void
clipman_actions_match_with_menu (ClipmanActions *actions,
                                 gint group,
                                 const gchar *text)
{
  ClipmanActionsEntry *entry;
  GtkWidget *mi;
  GSList *l, *entries;
  GdkModifierType state = 0;
  GdkDisplay* display = gdk_display_get_default ();
#if GTK_CHECK_VERSION (3, 20, 0)
  GdkSeat *seat = gdk_display_get_default_seat (display);
  GdkDevice *device = gdk_seat_get_pointer (seat);
#else
  GdkDeviceManager *device_manager = gdk_display_get_device_manager (display);
  GdkDevice *device = gdk_device_manager_get_client_pointer (device_manager);
#endif
  GdkScreen* screen = gdk_screen_get_default ();
  GdkWindow * root_win = gdk_screen_get_root_window (screen);

  if (group == ACTION_GROUP_SELECTION)
    {
      gint ctrl_mask = 0;

      gdk_window_get_device_position (root_win, device, NULL, NULL, &state);
      ctrl_mask = state & GDK_CONTROL_MASK;
      if (ctrl_mask && actions->priv->skip_action_on_key_down)
        {
          return;
        }
      else if (!ctrl_mask && !actions->priv->skip_action_on_key_down)
        {
          return;
        }
    }

  entries = clipman_actions_match (actions, group, text);

  if (entries == NULL)
    return;

  DBG ("Build the menu with actions");

  if (GTK_IS_MENU (actions->priv->menu))
    {
      gtk_widget_destroy (actions->priv->menu);
      actions->priv->menu = NULL;
    }

  actions->priv->menu = gtk_menu_new ();
  g_object_set_data_full (G_OBJECT (actions->priv->menu), "text", g_strdup (text), (GDestroyNotify)g_free);

  for (l = entries; l != NULL; l = l->next)
    {
      entry = l->data;

      mi = gtk_menu_item_new_with_label (entry->action_name);
      gtk_widget_set_sensitive (mi, FALSE);
      gtk_container_add (GTK_CONTAINER (actions->priv->menu), mi);

      mi = gtk_separator_menu_item_new ();
      gtk_container_add (GTK_CONTAINER (actions->priv->menu), mi);

        {
          GHashTableIter iter;
          gpointer key, value;
          g_hash_table_iter_init (&iter, entry->commands);
          while (g_hash_table_iter_next (&iter, &key, &value))
            {
              mi = gtk_menu_item_new_with_label ((const gchar *)key);
              g_object_set_data (G_OBJECT (mi), "text", g_object_get_data (G_OBJECT (actions->priv->menu), "text"));
              g_object_set_data (G_OBJECT (mi), "command", value);
              g_object_set_data (G_OBJECT (mi), "regex", entry->regex);
              gtk_container_add (GTK_CONTAINER (actions->priv->menu), mi);
              g_signal_connect (mi, "activate", G_CALLBACK (cb_entry_activated), NULL);
            }
        }

      mi = gtk_separator_menu_item_new ();
      gtk_container_add (GTK_CONTAINER (actions->priv->menu), mi);
    }

  mi = gtk_menu_item_new_with_label ("Cancel");
  gtk_container_add (GTK_CONTAINER (actions->priv->menu), mi);

  gtk_widget_show_all (actions->priv->menu);

  if(!gtk_widget_has_grab(actions->priv->menu))
  {
    gtk_grab_add(actions->priv->menu);
  }

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_menu_popup (GTK_MENU (actions->priv->menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
G_GNUC_END_IGNORE_DEPRECATIONS

  g_slist_free (entries);
}

/**
 * clipman_actions_load:
 * @actions:
 *
 */
void
clipman_actions_load (ClipmanActions *actions)
{
  gchar *filename;
  gchar *data;
  gssize size;
  gboolean load;
  GMarkupParseContext *context;
  EntryParser *parser;

  load = g_file_load_contents (actions->priv->file, NULL, &data, (gsize*)&size, NULL, NULL);

  if (!load)
    {
      /* Create user directory early to be sure it exists for next actions */
      GFile *dir = g_file_get_parent (actions->priv->file);
      g_file_make_directory_with_parents (dir, NULL, NULL);
      g_object_unref (dir);
      dir = NULL;

      /* Load from system wide file */
      filename = g_strdup (SYSCONFDIR"/xdg/xfce4/panel/xfce4-clipman-actions.xml");
      load = g_file_get_contents (filename, &data, (gsize*)&size, NULL);
      g_free (filename);
    }

  if (!load)
    {
      g_warning ("Unable to load actions from an XML file");
      return;
    }

  DBG ("Load actions from file");

  parser = g_slice_new0 (EntryParser);
  parser->actions = actions;
  parser->locale = setlocale (LC_MESSAGES, NULL);
  context = g_markup_parse_context_new (&markup_parser, 0, parser, NULL);
  g_markup_parse_context_parse (context, data, size, NULL);
  if (!g_markup_parse_context_end_parse (context, NULL))
    g_warning ("Error parsing the XML file");
  g_markup_parse_context_free (context);
  g_slice_free (EntryParser, parser);

  g_free (data);
}

/**
 * clipman_actions_save:
 * @actions:
 *
 */
void
clipman_actions_save (ClipmanActions *actions)
{
  ClipmanActionsEntry *entry;
  gchar *data;
  GString *output;
  gchar *tmp;
  GSList *l;

  /* Generate the XML output format */
  output = g_string_new ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                         "<actions>\n");

  for (l = actions->priv->entries; l != NULL; l = l->next)
    {
      entry = l->data;

      g_string_append (output, "\t<action>\n");

      tmp = g_markup_escape_text (entry->action_name, -1);
      g_string_append_printf (output, "\t\t<name>%s</name>\n", tmp);
      g_free (tmp);

      tmp = g_markup_escape_text (entry->pattern, -1);
      g_string_append_printf (output, "\t\t<regex>%s</regex>\n", tmp);
      g_free (tmp);

      g_string_append_printf (output, "\t\t<group>%d</group>\n", entry->group);

      g_string_append (output, "\t\t<commands>\n");

        {
          GHashTableIter iter;
          gpointer key, value;
          g_hash_table_iter_init (&iter, entry->commands);
          while (g_hash_table_iter_next (&iter, &key, &value))
            {
              g_string_append (output, "\t\t\t<command>\n");

              tmp = g_markup_escape_text (key, -1);
              g_string_append_printf (output, "\t\t\t\t<name>%s</name>\n", tmp);
              g_free (tmp);

              tmp = g_markup_escape_text (value, -1);
              g_string_append_printf (output, "\t\t\t\t<exec>%s</exec>\n", tmp);
              g_free (tmp);

              g_string_append (output, "\t\t\t</command>\n");
            }
        }

      g_string_append (output, "\t\t</commands>\n");

      g_string_append (output, "\t</action>\n");
    }

  g_string_append (output, "</actions>");

  /* And now write output to the xml file */
  DBG ("Save actions to file");
  data = g_string_free (output, FALSE);
  if (!g_file_replace_contents (actions->priv->file, data, strlen (data), NULL, FALSE,
                                G_FILE_CREATE_NONE, NULL, NULL, NULL))
    g_warning ("Unable to write the actions to the XML file");

  g_free (data);
}

/**
 * clipman_actions_get:
 *
 */
ClipmanActions *
clipman_actions_get (void)
{
  static ClipmanActions *singleton = NULL;

  if (singleton == NULL)
    {
      singleton = g_object_new (CLIPMAN_TYPE_ACTIONS, NULL);
      g_object_add_weak_pointer (G_OBJECT (singleton), (gpointer)&singleton);
    }
  else
    g_object_ref (G_OBJECT (singleton));

  return singleton;
}

/*
 * GObject
 */

static void
clipman_actions_class_init (ClipmanActionsClass *klass)
{
  GObjectClass *object_class;

  clipman_actions_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = clipman_actions_finalize;
  object_class->set_property = clipman_actions_set_property;
  object_class->get_property = clipman_actions_get_property;

  g_object_class_install_property (object_class, SKIP_ACTION_ON_KEY_DOWN,
                                   g_param_spec_boolean ("skip-action-on-key-down",
                                                         "SkipActionOnKeyDown",
                                                         "Skip the action if the Control key is pressed down",
                                                         DEFAULT_SKIP_ACTION_ON_KEY_DOWN,
                                                         G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
}

static void
clipman_actions_init (ClipmanActions *actions)
{
  gchar *filename;

  actions->priv = clipman_actions_get_instance_private (actions);

  /* Actions file */
  filename = g_strdup_printf ("%s/xfce4/panel/xfce4-clipman-actions.xml", g_get_user_config_dir ());
  actions->priv->file = g_file_new_for_path (filename);
  g_free (filename);

  /* Load initial actions */
  clipman_actions_load (actions);

  /* Listen on xml file changes */
  actions->priv->file_monitor = g_file_monitor_file (actions->priv->file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_signal_connect_swapped (actions->priv->file_monitor, "changed", G_CALLBACK (cb_file_changed), actions);
}

static void
clipman_actions_finalize (GObject *object)
{
  ClipmanActions *actions = CLIPMAN_ACTIONS (object);
  _clipman_actions_free_list (actions);
  g_object_unref (actions->priv->file_monitor);
  g_object_unref (actions->priv->file);
}

static void
clipman_actions_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  ClipmanActionsPrivate *priv = CLIPMAN_ACTIONS (object)->priv;

  switch (property_id)
    {
    case SKIP_ACTION_ON_KEY_DOWN:
      priv->skip_action_on_key_down = g_value_get_boolean (value);
      break;

    default:
      break;
    }
}

static void
clipman_actions_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  ClipmanActionsPrivate *priv = CLIPMAN_ACTIONS (object)->priv;

  switch (property_id)
    {
    case SKIP_ACTION_ON_KEY_DOWN:
      g_value_set_boolean (value, priv->skip_action_on_key_down);
      break;

    default:
      break;
    }
}
