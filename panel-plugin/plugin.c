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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gstdio.h>
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
 * Popup command
 */

static gboolean         my_plugin_set_popup_selection   (MyPlugin *plugin);
static GdkFilterReturn  event_filter_popup_menu         (GdkXEvent *xevent,
                                                         GdkEvent *event,
                                                         MyPlugin *plugin);
static gboolean         xfce_popup_grab_available       (GdkWindow *win,
                                                         guint32 timestamp);



static gboolean
clipboard_manager_ownership_exists (void)
{
  Display *display;
  Atom atom;

  display = gdk_x11_get_default_xdisplay ();
  atom = XInternAtom (display, "CLIPBOARD_MANAGER", FALSE);
  return XGetSelectionOwner (display, atom);
}

/*
 * Plugin functions
 */

MyPlugin *
plugin_register (void)
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
  xfconf_g_property_bind  (plugin->channel, "/tweaks/skip-action-on-key-down",
                           G_TYPE_BOOLEAN, plugin->actions, "skip-action-on-key-down");

  /* ClipmanHistory */
  plugin->history = clipman_history_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/max-texts-in-history",
                          G_TYPE_UINT, plugin->history, "max-texts-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/max-images-in-history",
                          G_TYPE_UINT, plugin->history, "max-images-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/save-on-quit",
                          G_TYPE_BOOLEAN, plugin->history, "save-on-quit");
  xfconf_g_property_bind (plugin->channel, "/tweaks/reorder-items",
                          G_TYPE_BOOLEAN, plugin->history, "reorder-items");

  /* ClipmanCollector */
  plugin->collector = clipman_collector_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/add-primary-clipboard",
                          G_TYPE_BOOLEAN, plugin->collector, "add-primary-clipboard");
  xfconf_g_property_bind (plugin->channel, "/settings/history-ignore-primary-clipboard",
                          G_TYPE_BOOLEAN, plugin->collector, "history-ignore-primary-clipboard");
  xfconf_g_property_bind (plugin->channel, "/settings/enable-actions",
                          G_TYPE_BOOLEAN, plugin->collector, "enable-actions");
  xfconf_g_property_bind (plugin->channel, "/tweaks/inhibit",
                          G_TYPE_BOOLEAN, plugin->collector, "inhibit");

  /* ClipmanMenu */
  plugin->menu = clipman_menu_new ();
#ifdef HAVE_QRENCODE
  xfconf_g_property_bind (plugin->channel, "/settings/show-qr-code",
                          G_TYPE_BOOLEAN, plugin->menu, "show-qr-code");
#endif
  xfconf_g_property_bind (plugin->channel, "/tweaks/reverse-menu-order",
                          G_TYPE_BOOLEAN, plugin->menu, "reverse-order");
  xfconf_g_property_bind (plugin->channel, "/tweaks/paste-on-activate",
                          G_TYPE_UINT, plugin->menu, "paste-on-activate");
  xfconf_g_property_bind (plugin->channel, "/tweaks/never-confirm-history-clear",
                          G_TYPE_BOOLEAN, plugin->menu, "never-confirm-history-clear");

  /* Load the data */
  plugin_load (plugin);

  /* Connect signal to save content */
  g_signal_connect_swapped (plugin->history, "item-added",
                            G_CALLBACK (plugin_save), plugin);
  g_signal_connect_swapped (plugin->history, "clear",
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
  gchar *dirname;
  const gchar *name;
  gint n_texts, n_images;
  gboolean save_on_quit;
  GDir *dir;

  /* Create initial directory and remove cache files */
  dirname = xfce_resource_save_location (XFCE_RESOURCE_CACHE, "xfce4/clipman/", TRUE);

  dir = g_dir_open (dirname, 0, NULL);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      filename = g_build_filename (dirname, name, NULL);
      g_unlink (filename);
      g_free (filename);
    }
  g_dir_close (dir);

  g_free (dirname);

  /* Return if the history must not be saved */
  g_object_get (plugin->history, "save-on-quit", &save_on_quit, NULL);
  if (save_on_quit == FALSE)
    return;

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
  gdk_window_remove_filter (gtk_widget_get_window(plugin->menu), (GdkFilterFunc) event_filter_popup_menu, plugin);
  gtk_widget_destroy (plugin->menu);
  g_object_unref (plugin->channel);
  g_object_unref (plugin->actions);
  g_object_unref (plugin->collector);
  g_object_unref (plugin->history);

#ifdef PANEL_PLUGIN
  gtk_widget_destroy (plugin->button);
#elif defined (STATUS_ICON)
  gtk_widget_destroy (plugin->popup_menu);
#endif

  g_slice_free (MyPlugin, plugin);
  xfconf_shutdown ();
}

void
plugin_about (MyPlugin *plugin)
{
  const gchar *authors[] = { "(c) 2008-2014 Mike Massonnet",
                             "(c) 2005-2006 Nick Schermer",
                             "(c) 2003 Eduard Roccatello",
                             "",
                             _("Contributors:"),
                             "(c) 2008-2009 David Collins",
                             "(c) 2013 Christian Hesse",
                             NULL, };
  const gchar *documenters[] = { "Mike Massonnet", NULL, };
  const gchar *license =
    "This program is free software; you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation; either version 2 of the License, or\n"
    "(at your option) any later version.\n";

  gtk_show_about_dialog (NULL,
                         "program-name", _("Clipman"),
                         "logo-icon-name", "xfce4-clipman-plugin",
                         "comments", _("Clipboard Manager for Xfce"),
                         "version", PACKAGE_VERSION,
                         "copyright", "Copyright © 2003-2014 The Xfce development team",
                         "license", license,
                         "website", "http://goodies.xfce.org/projects/panel-plugins/xfce4-clipman-plugin",
                         "website-label", "goodies.xfce.org",
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

  g_spawn_command_line_async ("xfce4-clipman-settings", &error);
  if (error != NULL)
  {
    error_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                           _("Unable to open the settings dialog"));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog), "%s", error->message);
    gtk_dialog_run (GTK_DIALOG (error_dialog));
    gtk_widget_destroy (error_dialog);
    g_error_free (error);
  }
}

void
plugin_popup_menu (MyPlugin *plugin)
{
#ifdef PANEL_PLUGIN
  gtk_menu_set_screen (GTK_MENU (plugin->menu), gtk_widget_get_screen (plugin->button));
  gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                  plugin->menu_position_func, plugin,
                  0, gtk_get_current_event_time ());
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), TRUE);
  xfce_panel_plugin_register_menu (plugin->panel_plugin, GTK_MENU (plugin->menu));
#elif defined (STATUS_ICON)
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_menu_set_screen (GTK_MENU (plugin->menu), gtk_status_icon_get_screen (plugin->status_icon));
G_GNUC_END_IGNORE_DEPRECATIONS
  usleep(100000);
  gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                  plugin->menu_position_func, plugin->status_icon,
                  0, gtk_get_current_event_time ());
#endif
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
  Display            *display;
  GdkWindow          *window;

  win = gtk_invisible_new ();
  gtk_widget_realize (win);
  id = GDK_WINDOW_XID (gtk_widget_get_window (win));
  display = gdk_x11_get_default_xdisplay ();

  gscreen = gtk_widget_get_screen (win);
  selection_name = g_strdup_printf (XFCE_CLIPMAN_SELECTION"%d",
                                    gdk_screen_get_number (gscreen));
  selection_atom = XInternAtom (display, selection_name, FALSE);
  g_free(selection_name);

  if (XGetSelectionOwner (display, selection_atom))
    {
      gtk_widget_destroy (win);
      return FALSE;
    }

  XSelectInput (display, id, PropertyChangeMask);
  XSetSelectionOwner (display, selection_atom, id, GDK_CURRENT_TIME);

  window = gtk_widget_get_window (win);
  gdk_window_add_filter (window, (GdkFilterFunc) event_filter_popup_menu, plugin);

  return TRUE;
}

static GdkFilterReturn
event_filter_popup_menu (GdkXEvent *xevent, GdkEvent *event, MyPlugin *plugin)
{
    XClientMessageEvent *evt;
    GdkScreen *screen;
    GdkWindow *root;
    Atom message_type;
    evt = (XClientMessageEvent *)xevent;

    if (((XEvent *)xevent)->type != ClientMessage)
      return GDK_FILTER_CONTINUE;

    message_type = XInternAtom (gdk_x11_get_default_xdisplay (), "STRING", FALSE);
    if (evt->message_type != message_type)
      return GDK_FILTER_CONTINUE;

    /* Copy workaround from xfdesktop to handle the awkward case where binding
     * a keyboard shortcut to the popup command doesn't always work out... */
#ifdef PANEL_PLUGIN
    screen = gtk_widget_get_screen (GTK_WIDGET (plugin->button));
#elif defined (STATUS_ICON)
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    screen = gtk_status_icon_get_screen (plugin->status_icon);
G_GNUC_END_IGNORE_DEPRECATIONS
#endif
    root = gdk_screen_get_root_window (screen);

    if (!xfce_popup_grab_available (root, GDK_CURRENT_TIME))
      {
        g_critical ("Unable to get keyboard/mouse grab.");
        return FALSE;
      }

  if (G_LIKELY (evt->format == 8) && (*(evt->data.b) != '\0'))
    {

      if (!g_ascii_strcasecmp (XFCE_CLIPMAN_MESSAGE, evt->data.b))
        {
          DBG ("Message received: %s", evt->data.b);

          if (xfconf_channel_get_bool (plugin->channel, "/tweaks/popup-at-pointer", FALSE))
            {
              gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL, NULL, NULL,
                              0, gtk_get_current_event_time ());
            }
          else
            {
              plugin_popup_menu (plugin);
            }

          return TRUE;
        }
    }

  return FALSE;
}

/* Code taken from xfwm4/src/menu.c:grab_available().  This should fix the case
 * where binding 'xfdesktop -menu' to a keyboard shortcut sometimes works and
 * sometimes doesn't.  Credit for this one goes to Olivier.
 */
static gboolean
xfce_popup_grab_available (GdkWindow *win, guint32 timestamp)
{
    GdkEventMask mask =
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
        GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
        GDK_POINTER_MOTION_MASK;
    GdkDisplay* display = gdk_window_get_display(win);
    GdkDeviceManager *device_manager = gdk_display_get_device_manager(display);
    GdkDevice* device = gdk_device_manager_get_client_pointer(device_manager);
    GdkGrabStatus g;
    gboolean grab_failed = FALSE;
    gint i = 0;

    TRACE ("entering grab_available");

    g = gdk_device_grab (device, win, GDK_OWNERSHIP_WINDOW, TRUE, mask, NULL, timestamp);

    while ((i++ < 2500) && (grab_failed = (g != GDK_GRAB_SUCCESS)))
    {
        TRACE ("grab not available yet, waiting... (%i)", i);
        g_usleep (100);
        if (g != GDK_GRAB_SUCCESS)
            g = gdk_device_grab (device, win, GDK_OWNERSHIP_WINDOW, TRUE, mask, NULL, timestamp);
    }

    if (g == GDK_GRAB_SUCCESS)
    {
        gdk_device_ungrab (device, timestamp);
    }

    return (!grab_failed);
}
