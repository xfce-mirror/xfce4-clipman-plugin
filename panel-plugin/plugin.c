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

#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <xfconf/xfconf.h>
#include <libxfce4util/libxfce4util.h>

#ifdef PANEL_PLUGIN
#include <libxfce4panel/libxfce4panel.h>
#endif

#include "common.h"
#include "plugin.h"
#include "actions.h"
#include "collector.h"
#include "history.h"
#include "menu.h"

/*
 * X11 Selection for the popup command
 */

static gboolean         my_plugin_set_popup_selection   (MyPlugin *plugin);
static gboolean         cb_popup_message_received       (MyPlugin *plugin,
                                                         GdkEventClient *ev);



static gboolean
clipboard_manager_ownership_exists ()
{
  Display *display;
  Atom atom;

  display = GDK_DISPLAY ();
  atom = XInternAtom (display, "CLIPBOARD_MANAGER", FALSE);
  return XGetSelectionOwner (display, atom);
}

/*
 * Plugin functions
 */

MyPlugin *
plugin_register ()
{
  MyPlugin *plugin = g_slice_new0 (MyPlugin);

  /* Locale */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, NULL);

  /* Daemon */
  if (!clipboard_manager_ownership_exists ())
    {
      plugin->daemon = gsd_clipboard_manager_new ();
      gsd_clipboard_manager_start (plugin->daemon, NULL);
    }

  /* Xfconf */
  xfconf_init (NULL);
  plugin->channel = xfconf_channel_new_with_property_base ("xfce4-panel", "/plugins/clipman");

  /* ClipmanActions */
  plugin->actions = clipman_actions_get ();

  /* ClipmanHistory */
  plugin->history = clipman_history_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/max-texts-in-history",
                          G_TYPE_UINT, plugin->history, "max-texts-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/max-images-in-history",
                          G_TYPE_UINT, plugin->history, "max-images-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/save-on-quit",
                          G_TYPE_BOOLEAN, plugin->history, "save-on-quit");

  /* ClipmanCollector */
  plugin->collector = clipman_collector_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/add-primary-clipboard",
                          G_TYPE_BOOLEAN, plugin->collector, "add-primary-clipboard");
  xfconf_g_property_bind (plugin->channel, "/settings/history-ignore-primary-clipboard",
                          G_TYPE_BOOLEAN, plugin->collector, "history-ignore-primary-clipboard");
  xfconf_g_property_bind (plugin->channel, "/settings/enable-actions",
                          G_TYPE_BOOLEAN, plugin->collector, "enable-actions");

  /* ClipmanMenu */
  plugin->menu = clipman_menu_new ();

  /* Load the data */
  plugin_load (plugin);

  /* Connect signal to save content */
  g_signal_connect_swapped (plugin->history, "item-added",
                            G_CALLBACK (plugin_save), plugin);

  /* Set the selection for the popup command */
  my_plugin_set_popup_selection (plugin);

  return plugin;
}

void
plugin_load (MyPlugin *plugin)
{
  GKeyFile *keyfile;
  gchar **texts = NULL;
  gchar *filename;
  GdkPixbuf *image;
  gint i = 0;
  gboolean save_on_quit;

  /* Return if the history must not be saved */
  g_object_get (plugin->history, "save-on-quit", &save_on_quit, NULL);
  if (save_on_quit == FALSE)
    return;

  /* Load images */
  while (TRUE)
    {
      filename = g_strdup_printf ("%s/xfce4/clipman/image%d.png", g_get_user_cache_dir (), i++);
      image = gdk_pixbuf_new_from_file (filename, NULL);
      g_unlink (filename);
      g_free (filename);
      if (image == NULL)
        break;

      DBG ("Loading image from cache file %s", filename);
      clipman_history_add_image (plugin->history, image);
      g_object_unref (image);
    }

  /* Load texts */
  filename = g_strdup_printf ("%s/xfce4/clipman/textsrc", g_get_user_cache_dir ());
  DBG ("Loading texts from cache file %s", filename);
  keyfile = g_key_file_new ();
  if (g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, NULL))
    {
      texts = g_key_file_get_string_list (keyfile, "texts", "texts", NULL, NULL);
      for (i = 0; texts != NULL && texts[i] != NULL; i++)
        clipman_history_add_text (plugin->history, texts[i]);
      g_unlink (filename);
    }

  g_key_file_free (keyfile);
  g_strfreev (texts);
  g_free (filename);

  /* Set no current item */
  clipman_history_set_item_to_restore (plugin->history, NULL);
}

void
plugin_save (MyPlugin *plugin)
{
  GSList *list, *l;
  const ClipmanHistoryItem *item;
  GKeyFile *keyfile;
  const gchar **texts;
  gchar *data;
  gchar *filename;
  gint n_texts, n_images;
  gboolean save_on_quit;

  /* Return if the history must not be saved */
  g_object_get (plugin->history, "save-on-quit", &save_on_quit, NULL);
  if (save_on_quit == FALSE)
    return;

  /* Create initial directory */
  filename = xfce_resource_save_location (XFCE_RESOURCE_CACHE, "xfce4/clipman/", TRUE);
  g_free (filename);

  /* Save the history */
  list = clipman_history_get_list (plugin->history);
  list = g_slist_reverse (list);
  if (list != NULL)
    {
      texts = g_malloc0 (g_slist_length (list) * sizeof (gchar *));
      for (n_texts = n_images = 0, l = list; l != NULL; l = l->next)
        {
          item = l->data;

          switch (item->type)
            {
            case CLIPMAN_HISTORY_TYPE_TEXT:
              texts[n_texts++] = item->content.text;
              break;

            case CLIPMAN_HISTORY_TYPE_IMAGE:
              filename = g_strdup_printf ("%s/xfce4/clipman/image%d.png", g_get_user_cache_dir (), n_images++);
              if (!gdk_pixbuf_save (item->content.image, filename, "png", NULL, NULL))
                g_warning ("Failed to save image to cache file %s", filename);
              else
                DBG ("Saved image to cache file %s", filename);
              g_free (filename);
              break;

            default:
              g_assert_not_reached ();
            }
        }

      if (n_texts > 0)
        {
          filename = g_strdup_printf ("%s/xfce4/clipman/textsrc", g_get_user_cache_dir ());
          keyfile = g_key_file_new ();
          g_key_file_set_string_list (keyfile, "texts", "texts", texts, n_texts);
          data = g_key_file_to_data (keyfile, NULL, NULL);
          g_file_set_contents (filename, data, -1, NULL);
          DBG ("Saved texts to cache file %s", filename);

          g_key_file_free (keyfile);
          g_free (data);
          g_free (filename);
        }

      g_free (texts);
      g_slist_free (list);
    }
}

void
plugin_free (MyPlugin *plugin)
{
  if (plugin->daemon != NULL)
    {
      gsd_clipboard_manager_stop (plugin->daemon);
      g_object_unref (plugin->daemon);
    }
  gtk_widget_destroy (plugin->menu);
  g_object_unref (plugin->channel);
  g_object_unref (plugin->actions);
  g_object_unref (plugin->collector);
  g_object_unref (plugin->history);

#ifdef PANEL_PLUGIN
  gtk_widget_destroy (plugin->button);
#elif STATUS_ICON
  gtk_widget_destroy (plugin->popup_menu);
#endif

  g_slice_free (MyPlugin, plugin);
  xfconf_shutdown ();
}

static void
cb_about_dialog_url_hook (GtkAboutDialog *dialog,
                          const gchar *uri,
                          gpointer user_data)
{
  gchar *command;

  command = g_strdup_printf ("exo-open %s", uri);
  if (!g_spawn_command_line_async (command, NULL))
    {
      g_free (command);
      command = g_strdup_printf ("firefox %s", uri);
      g_spawn_command_line_async (command, NULL);
    }
  g_free (command);
}

void
plugin_about (MyPlugin *plugin)
{
  const gchar *artists[] = { "Mike Massonnet", NULL, };
  const gchar *authors[] = { "(c) 2008-2009 Mike Massonnet",
                             "(c) 2005-2006 Nick Schermer",
                             "(c) 2003 Eduard Roccatello",
                             NULL, };
  const gchar *documenters[] = { "Mike Massonnet", NULL, };
  const gchar *license =
    "This program is free software; you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation; either version 2 of the License, or\n"
    "(at your option) any later version.\n";

  gtk_about_dialog_set_url_hook (cb_about_dialog_url_hook, NULL, NULL);
  gtk_show_about_dialog (NULL,
                         "program-name", _("Clipman"),
                         "logo-icon-name", "xfce4-clipman-plugin",
                         "comments", _("Clipboard Manager for Xfce"),
                         "version", PACKAGE_VERSION,
                         "copyright", "Copyright © 2003-2009 The Xfce development team",
                         "license", license,
                         "website", "http://goodies.xfce.org/projects/panel-plugins/xfce4-clipman-plugin",
                         "website-label", "goodies.xfce.org",
                         "artists", artists,
                         "authors", authors,
                         "documenters", documenters,
                         "translator-credits", _("translator-credits"),
                         NULL);
}

void
plugin_configure (MyPlugin *plugin)
{
  GError *error = NULL;
  GtkWidget *error_dialog;

  gdk_spawn_command_line_on_screen (gdk_screen_get_default (), "xfce4-clipman-settings", &error);
  if (error != NULL)
  {
    error_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                           _("Unable to open the settings dialog"), NULL);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog), "%s", error->message, NULL);
    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (error_dialog);
    g_error_free (error);
  }
}

/*
 * X11 Selection for the popup command
 */

static gboolean
my_plugin_set_popup_selection (MyPlugin *plugin)
{
  GdkScreen          *gscreen;
  gchar              *selection_name;
  Atom                selection_atom;
  GtkWidget          *win;
  Window              id;

  win = gtk_invisible_new ();
  gtk_widget_realize (win);
  id = GDK_WINDOW_XID (win->window);

  gscreen = gtk_widget_get_screen (win);
  selection_name = g_strdup_printf (XFCE_CLIPMAN_SELECTION"%d",
                                    gdk_screen_get_number (gscreen));
  selection_atom = XInternAtom (GDK_DISPLAY(), selection_name, FALSE);

  if (XGetSelectionOwner (GDK_DISPLAY(), selection_atom))
    {
      gtk_widget_destroy (win);
      return FALSE;
    }

  XSelectInput (GDK_DISPLAY(), id, PropertyChangeMask);
  XSetSelectionOwner (GDK_DISPLAY(), selection_atom, id, GDK_CURRENT_TIME);

  g_signal_connect_swapped (win, "client-event",
                            G_CALLBACK (cb_popup_message_received), plugin);

  return TRUE;
}

static gboolean
cb_popup_message_received (MyPlugin *plugin,
                           GdkEventClient *ev)
{
  if (G_LIKELY (ev->data_format == 8 && *(ev->data.b) != '\0'))
    {
      if (!g_ascii_strcasecmp (XFCE_CLIPMAN_MESSAGE, ev->data.b))
        {
          DBG ("Message received: %s", ev->data.b);

#ifdef PANEL_PLUGIN
          xfce_panel_plugin_set_panel_hidden (plugin->panel_plugin, FALSE);
#endif
          while (gtk_events_pending ())
            gtk_main_iteration ();

#ifdef PANEL_PLUGIN
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), TRUE);
#elif STATUS_ICON
          g_signal_emit_by_name (plugin->status_icon, "activate", NULL);
#endif
          return TRUE;
        }
    }
  return FALSE;
}

