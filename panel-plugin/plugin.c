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

#ifdef HAVE_XFCE_REVISION_H
#include "xfce-revision.h"
#endif

#include "common.h"
#include "plugin.h"

#include <glib/gstdio.h>
#include <libxfce4util/libxfce4util.h>



/*
 * Plugin actions
 */

static void
plugin_action_set_text (GSimpleAction *action,
                        GVariant *value,
                        gpointer data)
{
  gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), g_variant_get_string (value, NULL), -1);
}

static const GActionEntry plugin_actions[] = {
  { "set-text", plugin_action_set_text, "s", NULL, NULL },
};



/*
 * Plugin functions
 */

static void
plugin_clear (MyPlugin *plugin)
{
  gchar *dirname = xfce_resource_save_location (XFCE_RESOURCE_CACHE, "xfce4/clipman/", FALSE);
  GDir *dir = g_dir_open (dirname, 0, NULL);
  if (dir != NULL)
    {
      const gchar *name;
      while ((name = g_dir_read_name (dir)) != NULL)
        {
          gchar *filename = g_build_filename (dirname, name, NULL);
          g_unlink (filename);
          g_free (filename);
        }
      g_dir_close (dir);
    }
  g_free (dirname);
}

MyPlugin *
plugin_register (void)
{
  MyPlugin *plugin;
  GtkApplication *app;
  GError *error = NULL;

  /* Locale */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, NULL);

  /* Xfconf */
  if (!xfconf_init (&error))
    {
      g_critical ("Xfconf initialization failed: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  app = gtk_application_new ("org.xfce.clipman", G_APPLICATION_FLAGS_NONE);
  if (!g_application_register (G_APPLICATION (app), NULL, &error))
    {
      g_critical ("Unable to register GApplication: %s", error->message);
      g_error_free (error);
      g_object_unref (app);
      return NULL;
    }

  if (g_application_get_is_remote (G_APPLICATION (app)))
    {
      g_message ("Primary instance org.xfce.clipman already running");
      clipman_common_show_info_dialog ();
      g_object_unref (app);
      return NULL;
    }

  g_set_application_name (_("Clipman"));
  plugin = g_slice_new0 (MyPlugin);
  plugin->app = app;
  g_signal_connect_swapped (plugin->app, "activate", G_CALLBACK (plugin_popup_menu), plugin);
  g_action_map_add_action_entries (G_ACTION_MAP (app), plugin_actions, G_N_ELEMENTS (plugin_actions), plugin);
  plugin->channel = xfconf_channel_new_with_property_base ("xfce4-panel", "/plugins/clipman");

  /* Daemon */
  plugin->daemon = xcp_clipboard_manager_get ();

  /* ClipmanActions */
  plugin->actions = clipman_actions_get ();
  xfconf_g_property_bind (plugin->channel, "/tweaks/skip-action-on-key-down",
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
  xfconf_g_property_bind (plugin->channel, "/settings/persistent-primary-clipboard",
                          G_TYPE_BOOLEAN, plugin->collector, "persistent-primary-clipboard");
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
  xfconf_g_property_bind (plugin->channel, "/tweaks/max-menu-items",
                          G_TYPE_UINT, plugin->menu, "max-menu-items");
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
                            G_CALLBACK (plugin_clear), plugin);

  return plugin;
}

static gint
compare_images (gconstpointer a,
                gconstpointer b)
{
  gint pos_a = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (a), "image-pos"));
  gint pos_b = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (b), "image-pos"));
  return pos_a < pos_b ? -1 : (pos_a > pos_b ? 1 : 0);
}

static gint
add_next_image (ClipmanHistory *history,
                GList **_images)
{
  GList *images = *_images;
  ClipmanHistoryItem *item = clipman_history_add_image (history, images->data);
  if (item != NULL)
    item->filename = g_strdup (g_object_get_data (G_OBJECT (images->data), "filename"));
  g_object_unref (images->data);
  *_images = images = g_list_delete_link (images, images);
  if (images != NULL)
    return GPOINTER_TO_INT (g_object_get_data (images->data, "image-pos"));

  return -1;
}

void
plugin_load (MyPlugin *plugin)
{
  GKeyFile *keyfile;
  GList *images = NULL;
  GDir *dir;
  gchar *dirname, *filename;
  gboolean save_on_quit;

  /* Return if the history must not be saved */
  g_object_get (plugin->history, "save-on-quit", &save_on_quit, NULL);
  if (save_on_quit == FALSE)
    return;

  dirname = xfce_resource_save_location (XFCE_RESOURCE_CACHE, "xfce4/clipman/", FALSE);

  /* Load images */
  dir = g_dir_open (dirname, 0, NULL);
  if (dir != NULL)
    {
      const gchar *name;
      while ((name = g_dir_read_name (dir)) != NULL)
        {
          if (g_str_has_prefix (name, "image"))
            {
              GError *error = NULL;
              GdkPixbuf *image;

              filename = g_build_filename (dirname, name, NULL);
              image = gdk_pixbuf_new_from_file (filename, &error);
              if (image == NULL)
                {
                  g_warning ("Failed to load image from cache file %s: %s", filename, error->message);
                  g_error_free (error);
                }
              else
                {
                  DBG ("Loaded image from cache file %s", filename);
                  g_object_set_data_full (G_OBJECT (image), "image-name", g_strdup (name), g_free);
                  g_object_set_data_full (G_OBJECT (image), "filename", g_strdup (filename), g_free);
                  images = g_list_prepend (images, image);
                }
              g_free (filename);
            }
        }
      g_dir_close (dir);
    }

  /* Load texts */
  filename = g_build_filename (dirname, "textsrc", NULL);
  DBG ("Loading texts from cache file %s", filename);
  keyfile = g_key_file_new ();
  if (g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, NULL))
    {
      gchar **texts = g_key_file_get_string_list (keyfile, "texts", "texts", NULL, NULL);
      gint image_pos = -1;

      /* prepare image list */
      if (images != NULL)
        {
          for (GList *lp = images; lp != NULL; lp = lp->next)
            {
              const gchar *image_name = g_object_get_data (G_OBJECT (lp->data), "image-name");
              image_pos = g_key_file_get_integer (keyfile, "images", image_name, NULL);
              g_object_set_data (G_OBJECT (lp->data), "image-pos", GINT_TO_POINTER (image_pos));
            }
          images = g_list_sort (images, compare_images);
          image_pos = GPOINTER_TO_INT (g_object_get_data (images->data, "image-pos"));

          /* shouldn't happen: first add images that weren't found in keyfile (end of history) */
          while (image_pos == 0)
            image_pos = add_next_image (plugin->history, &images);
        }

      /* add texts and images in order */
      if (texts != NULL)
        {
          gint n_items = 0;
          for (gchar **text = texts; *text != NULL; n_items++)
            {
              if (n_items == image_pos)
                image_pos = add_next_image (plugin->history, &images);
              else
                clipman_history_add_text (plugin->history, *text++);
            }
          g_strfreev (texts);
        }

      /* add remaining images, if any (start of history) */
      while (images != NULL)
        add_next_image (plugin->history, &images);
    }

  g_list_free_full (images, g_object_unref);
  g_key_file_free (keyfile);
  g_free (filename);
  g_free (dirname);
}

void
plugin_save (MyPlugin *plugin)
{
  GSList *list;
  gchar *dirname;
  gboolean save_on_quit;

  /* Return if the history must not be saved */
  g_object_get (plugin->history, "save-on-quit", &save_on_quit, NULL);
  if (save_on_quit == FALSE)
    return;

  /* Create initial directory if needed */
  dirname = xfce_resource_save_location (XFCE_RESOURCE_CACHE, "xfce4/clipman/", TRUE);
  if (dirname == NULL)
    {
      g_warning ("Failed to create Clipman cache directory");
      return;
    }

  /* Save the history */
  list = clipman_history_get_list (plugin->history);
  if (list != NULL)
    {
      const gchar **texts = g_new0 (const gchar *, g_slist_length (list));
      GdkPoint image_pos[clipman_history_get_max_images_in_history (plugin->history)];
      gint n_items = 0, n_texts = 0, n_images = 0;

      for (GSList *l = g_slist_reverse (list); l != NULL; l = l->next, n_items++)
        {
          ClipmanHistoryItem *item = l->data;

          switch (item->type)
            {
            case CLIPMAN_HISTORY_TYPE_TEXT:
              texts[n_texts++] = item->content.text;
              break;

            case CLIPMAN_HISTORY_TYPE_IMAGE:
              if (item->filename == NULL)
                {
                  GError *error = NULL;
                  gint n = 0;
                  gchar *basename = g_strdup_printf ("image%d.png", n);

                  item->filename = g_build_filename (dirname, basename, NULL);
                  while (g_file_test (item->filename, G_FILE_TEST_EXISTS))
                    {
                      g_free (item->filename);
                      g_free (basename);
                      basename = g_strdup_printf ("image%d.png", ++n);
                      item->filename = g_build_filename (dirname, basename, NULL);
                    }

                  if (!gdk_pixbuf_save (item->content.image, item->filename, "png", &error, NULL))
                    {
                      g_warning ("Failed to save image to cache file %s: %s", item->filename, error->message);
                      g_error_free (error);
                      g_unlink (item->filename);
                      g_free (item->filename);
                      item->filename = NULL;
                    }
                  else
                    DBG ("Saved image to cache file %s", item->filename);

                  g_free (basename);
                }

              if (item->filename != NULL)
                {
                  image_pos[n_images].x = atoi (g_strrstr_len (item->filename, -1, "image") + 5);
                  image_pos[n_images].y = n_items;
                  n_images++;
                }
              break;

            default:
              g_assert_not_reached ();
            }
        }

      if (n_texts > 0 || n_images > 0)
        {
          GKeyFile *keyfile = g_key_file_new ();
          GError *error = NULL;
          gchar *filename = g_build_filename (dirname, "textsrc", NULL);

          if (n_texts > 0)
            g_key_file_set_string_list (keyfile, "texts", "texts", texts, n_texts);

          for (gint n = 0; n < n_images; n++)
            {
              gchar *basename = g_strdup_printf ("image%d.png", image_pos[n].x);
              g_key_file_set_integer (keyfile, "images", basename, image_pos[n].y);
              g_free (basename);
            }

          if (!g_key_file_save_to_file (keyfile, filename, &error))
            {
              g_warning ("Failed to save history to cache file %s: %s", filename, error->message);
              g_error_free (error);
            }
          else
            DBG ("Saved texts to cache file %s", filename);

          g_key_file_free (keyfile);
          g_free (filename);
        }

      g_free (texts);
      g_slist_free (list);
    }

  g_free (dirname);
}

void
plugin_free (MyPlugin *plugin)
{
  g_object_unref (plugin->daemon);
  g_object_unref (plugin->menu);
  g_object_unref (plugin->channel);
  g_object_unref (plugin->actions);
  g_object_unref (plugin->collector);
  g_object_unref (plugin->history);

#ifdef PANEL_PLUGIN
  gtk_widget_destroy (plugin->button);
#elif defined(STATUS_ICON)
  if (plugin->popup_menu != NULL)
    gtk_widget_destroy (plugin->popup_menu);
#endif

  g_object_unref (plugin->app);
  g_slice_free (MyPlugin, plugin);
  xfconf_shutdown ();
}

void
plugin_about (MyPlugin *plugin)
{
  const gchar *authors[] = {
    "(c) 2014-2020 Simon Steinbeiss",
    "(c) 2008-2014 Mike Massonnet",
    "(c) 2005-2006 Nick Schermer",
    "(c) 2003 Eduard Roccatello",
    "",
    _("Contributors:"),
    "(c) 2008-2009 David Collins",
    "(c) 2013 Christian Hesse",
    NULL,
  };
  const gchar *documenters[] = {
    "Mike Massonnet",
    NULL,
  };
  const gchar *license =
    "This program is free software; you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation; either version 2 of the License, or\n"
    "(at your option) any later version.\n";

  gtk_show_about_dialog (NULL,
                         "program-name", _("Clipman"),
                         "logo-icon-name", "xfce4-clipman-plugin",
                         "comments", _("Clipboard Manager for Xfce"),
                         "version", VERSION_FULL,
                         "copyright", "Copyright © 2003-" COPYRIGHT_YEAR " The Xfce development team",
                         "license", license,
                         "website", PACKAGE_URL,
                         "website-label", "docs.xfce.org",
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
  GdkEvent *event = gtk_get_current_event ();
  gboolean popup_command = (event == NULL);

  if (popup_command)
    {
      GdkSeat *seat = gdk_display_get_default_seat (gdk_display_get_default ());
      event = gdk_event_new (GDK_BUTTON_PRESS);
      event->button.window = g_object_ref (gdk_get_default_root_window ());
      gdk_event_set_device (event, gdk_seat_get_pointer (seat));
    }

  /* store clipboard contents for later use when the menu is shown: we can't do this
   * at that time as it iterates the main loop and it causes critical warnings */
  g_object_set_data_full (G_OBJECT (plugin->menu), "selection-clipboard",
                          gtk_clipboard_wait_for_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD)), g_free);
  g_object_set_data_full (G_OBJECT (plugin->menu), "selection-primary",
                          gtk_clipboard_wait_for_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY)), g_free);

  if (xfconf_channel_get_bool (plugin->channel, "/tweaks/popup-at-pointer", FALSE))
    {
#ifdef PANEL_PLUGIN
      if (!popup_command)
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), TRUE);
          xfce_panel_plugin_register_menu (plugin->panel_plugin, GTK_MENU (plugin->menu));
        }
#endif
      gtk_menu_popup_at_pointer (GTK_MENU (plugin->menu), event);
    }
  else
    {
#ifdef PANEL_PLUGIN
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), TRUE);
#if LIBXFCE4PANEL_CHECK_VERSION(4, 17, 2)
      xfce_panel_plugin_popup_menu (plugin->panel_plugin, GTK_MENU (plugin->menu), plugin->button, event);
#else
      xfce_panel_plugin_register_menu (plugin->panel_plugin, GTK_MENU (plugin->menu));
      gtk_menu_set_screen (GTK_MENU (plugin->menu), gtk_widget_get_screen (plugin->button));
      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                      plugin->menu_position_func, plugin,
                      0, gtk_get_current_event_time ());
      G_GNUC_END_IGNORE_DEPRECATIONS
#endif

#elif defined(STATUS_ICON)

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      gtk_menu_set_screen (GTK_MENU (plugin->menu),
                           gtk_status_icon_get_screen (plugin->status_icon));
      gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                      plugin->menu_position_func, plugin->status_icon,
                      0, gtk_get_current_event_time ());
      G_GNUC_END_IGNORE_DEPRECATIONS
#endif
    }

  gdk_event_free (event);
}
