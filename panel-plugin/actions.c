/*
 *  Copyright (c) 2009 Mike Massonnet <mmassonnet@xfce.org>
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

#include <exo/exo.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "common.h"

#include "actions.h"

/*
 * GObject declarations
 */

G_DEFINE_TYPE (ClipmanActions, clipman_actions, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CLIPMAN_TYPE_ACTIONS, ClipmanActionsPrivate))

struct _ClipmanActionsPrivate
{
  GSList               *entries;
  GtkWidget            *menu;
};

static void             clipman_actions_class_init          (ClipmanActionsClass *klass);
static void             clipman_actions_init                (ClipmanActions *actions);
static void             clipman_actions_finalize            (GObject *object);

/*
 * Misc functions declarations
 */

static gint           __clipman_actions_entry_compare       (gpointer a,
                                                             gpointer b);
static gint           __clipman_actions_entry_compare_name  (gpointer a,
                                                             gpointer b);
static void           __clipman_actions_entry_free          (ClipmanActionsEntry *entry);
#if !GLIB_CHECK_VERSION (2,16,0)
static void           __foreach_command_create_mi           (gpointer key,
                                                             gpointer value,
                                                             gpointer user_data);
static void           __foreach_command_write_xml           (gpointer key,
                                                             gpointer value,
                                                             gpointer user_data);
#endif

/*
 * Callbacks declarations
 */

static void             cb_entry_activated                  (GtkMenuItem *mi,
                                                             gpointer user_data);

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

      parser->state = ACTIONS;
      break;

    case ACTION_NAME:
    case REGEX:
    case COMMANDS:
      parser->state = ACTION;
      break;

    case COMMAND:
      if (parser->action_name == NULL || parser->regex == NULL)
        g_warning ("Closing a command but no action name nor regex set");
      else
        clipman_actions_add (parser->actions, parser->action_name, parser->regex,
                             parser->command_name, parser->command);

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

  xfce_exec (real_command, FALSE, FALSE, &error);
  if (error != NULL)
    {
      xfce_err (_("Unable to execute the command \"%s\"\n\n%s"), real_command, error->message);
      g_error_free (error);
    }
  g_free (real_command);
}

/*
 * Misc functions
 */

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
  g_regex_unref (entry->regex);
  g_hash_table_destroy (entry->commands);
  g_slice_free (ClipmanActionsEntry, entry);
}

#if !GLIB_CHECK_VERSION (2,16,0)
static void
__foreach_command_create_mi (gpointer key,
                             gpointer value,
                             gpointer user_data)
{
  gchar *command_name = key;
  gchar *command = value;
  GtkWidget *menu = user_data;
  GtkWidget *mi;

  mi = gtk_menu_item_new_with_label (command_name);
  g_object_set_data (G_OBJECT (mi), "text", g_object_get_data (G_OBJECT (menu), "text"));
  g_object_set_data (G_OBJECT (mi), "command", command);
  g_object_set_data (G_OBJECT (mi), "regex", g_object_get_data (G_OBJECT (menu), "regex"));
  gtk_container_add (GTK_CONTAINER (menu), mi);
  g_signal_connect (mi, "activate", G_CALLBACK (cb_entry_activated), NULL);
}

static void
__foreach_command_write_xml (gpointer key,
                             gpointer value,
                             gpointer user_data)
{
  gchar *command_name = key;
  gchar *command = value;
  GString *output = user_data;
  gchar *tmp;

  g_string_append (output, "\t\t\t<command>\n");

  tmp = g_markup_escape_text (command_name, -1);
  g_string_append_printf (output, "\t\t\t\t<name>%s</name>\n", tmp);
  g_free (tmp);

  tmp = g_markup_escape_text (command, -1);
  g_string_append_printf (output, "\t\t\t\t<exec>%s</exec>\n", tmp);
  g_free (tmp);

  g_string_append (output, "\t\t\t</command>\n");
}
#endif

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

  g_return_val_if_fail (G_LIKELY (action_name != NULL), FALSE);
  g_return_val_if_fail (G_LIKELY (command_name != NULL), FALSE);
  g_return_val_if_fail (G_LIKELY (command != NULL), FALSE);

  l = g_slist_find_custom (actions->priv->entries, action_name, (GCompareFunc)__clipman_actions_entry_compare_name);

  /* Add a new entry to the list */
  if (l == NULL)
    {
      /* Validate the regex */
      _regex = g_regex_new (regex, G_REGEX_CASELESS|G_REGEX_ANCHORED, 0, NULL);
      if (_regex == NULL)
        return FALSE;

      DBG ("New entry `%s' with command `%s'", action_name, command_name);

      entry = g_slice_new0 (ClipmanActionsEntry);
      entry->action_name = g_strdup (action_name);
      entry->regex = _regex;
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
 * @text:       the text to match against the existing regex's
 *
 * Searches a regex match for @text and returns a newly allocated #GSList which
 * must be freed with g_slist_free() that contains a list of
 * #ClipmanActionsEntry for each matched action.
 * Note that the data inside the list is owned by #ClipmanActions and must not
 * be modified.
 *
 * Returns: a newly allocated #GSList
 */
GSList *
clipman_actions_match (ClipmanActions *actions,
                       const gchar *text)
{
  ClipmanActionsEntry *entry;
  GSList *l;
  GSList *entries = NULL;

  for (l = actions->priv->entries; l != NULL; l = l->next)
    {
      entry = l->data;
      if (g_regex_match (entry->regex, text, 0, NULL))
        entries = g_slist_prepend (entries, entry);
    }

  return entries;
}

/**
 * clipman_actions_match_with_menu:
 * @actions:    a #ClipmanActions
 * @text:       the text to match against the existing regex's
 *
 * Builds and displays a menu with matching actions for @text.
 *
 * Read clipman_actions_match() for more information.
 */
void
clipman_actions_match_with_menu (ClipmanActions *actions,
                                 const gchar *text)
{
  ClipmanActionsEntry *entry;
  GtkWidget *mi;
  GSList *l, *entries;

  entries = clipman_actions_match (actions, text);

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

#if GLIB_CHECK_VERSION (2,16,0)
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
#else
      g_object_set_data (G_OBJECT (actions->priv->menu), "regex", entry->regex);
      g_hash_table_foreach (entry->commands, (GHFunc)__foreach_command_create_mi, actions->priv->menu);
#endif

      mi = gtk_separator_menu_item_new ();
      gtk_container_add (GTK_CONTAINER (actions->priv->menu), mi);
    }

  mi = gtk_menu_item_new_with_label ("Cancel");
  gtk_container_add (GTK_CONTAINER (actions->priv->menu), mi);

  gtk_widget_show_all (actions->priv->menu);
  gtk_menu_popup (GTK_MENU (actions->priv->menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());

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
  gsize size;
  gboolean load;
  GMarkupParseContext *context;
  EntryParser *parser;

  filename = g_strdup_printf ("%s/xfce4/panel/xfce4-clipman-actions.xml", g_get_user_config_dir ());
  load = g_file_get_contents (filename, &data, &size, NULL);

  if (!load)
    {
      g_free (filename);
      filename = g_strdup (SYSCONFDIR"/xdg/xfce4/panel/xfce4-clipman-actions.xml");
      load = g_file_get_contents (filename, &data, &size, NULL);
    }

  if (!load)
    g_warning ("Unable to load actions from an XML file");

  DBG ("Load actions from file %s", filename);

  if (load)
    {
      parser = g_slice_new0 (EntryParser);
      parser->actions = actions;
      parser->locale = setlocale (LC_MESSAGES, NULL);
      context = g_markup_parse_context_new (&markup_parser, 0, parser, NULL);
      g_markup_parse_context_parse (context, data, (gssize)size, NULL);
      if (!g_markup_parse_context_end_parse (context, NULL))
        g_warning ("Error parsing the XML file");
      g_markup_parse_context_free (context);
      g_slice_free (EntryParser, parser);
    }

  g_free (filename);
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
  gchar *filename;
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

      tmp = g_markup_escape_text (g_regex_get_pattern (entry->regex), -1);
      g_string_append_printf (output, "\t\t<regex>%s</regex>\n", tmp);
      g_free (tmp);

      g_string_append (output, "\t\t<commands>\n");

#if GLIB_CHECK_VERSION (2,16,0)
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
#else
      g_hash_table_foreach (entry->commands, (GHFunc)__foreach_command_write_xml, output);
#endif

      g_string_append (output, "\t\t</commands>\n");

      g_string_append (output, "\t</action>\n");
    }

  g_string_append (output, "</actions>");

  /* And now write output to the xml file */
  filename = g_strdup_printf ("%s/xfce4/panel/xfce4-clipman-actions.xml", g_get_user_config_dir ());
  DBG ("Save actions to file %s", filename);
  data = g_string_free (output, FALSE);
  if (!g_file_set_contents (filename, data, -1, NULL))
    g_warning ("Unable to write the actions to the XML file %s", filename);

  g_free (filename);
  g_free (data);
}

/**
 * clipman_actions_get:
 *
 */
ClipmanActions *
clipman_actions_get ()
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

  g_type_class_add_private (klass, sizeof (ClipmanActionsPrivate));

  clipman_actions_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = clipman_actions_finalize;
}

static void
clipman_actions_init (ClipmanActions *actions)
{
  actions->priv = GET_PRIVATE (actions);

  clipman_actions_load (actions);
}

static void
clipman_actions_finalize (GObject *object)
{
  ClipmanActions *actions = CLIPMAN_ACTIONS (object);
  GSList *l;

  for (l = actions->priv->entries; l != NULL; l = l->next)
    __clipman_actions_entry_free (l->data);
  g_slist_free (actions->priv->entries);
}

