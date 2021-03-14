/*
 *  Copyright (c) 2008-2012 Mike Massonnet <mmassonnet@xfce.org>
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

#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

#include "common.h"
#include "plugin.h"

/*
 * Status Icon
 */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static MyPlugin*        status_icon_register            (void);
static void             cb_status_icon_activate         (MyPlugin *plugin);
static void             cb_status_icon_popup_menu       (MyPlugin *plugin,
                                                         guint button,
                                                         guint activate_time);
static void             cb_status_icon_quit             (MyPlugin *plugin);
static void             cb_status_icon_finalize         (gpointer  data,
                                                         GObject  *where_the_object_was);
static void             install_autostart_file          (void);
static void             update_autostart_file           (gboolean autostart);

/*
 * Plugin Registration
 */

gint
main (gint argc,
      gchar *argv[])
{
  MyPlugin *plugin;
  GtkApplication *app;
  GError *error = NULL;

  gtk_init (&argc, &argv);
  app = gtk_application_new ("org.xfce.clipman", 0);

  g_application_register (G_APPLICATION (app), NULL, &error);
  if (error != NULL)
    {
      g_warning ("Unable to register GApplication: %s", error->message);
      g_error_free (error);
      error = NULL;
    }

  if (g_application_get_is_remote (G_APPLICATION (app)))
    {
      g_message ("Primary instance org.xfce.clipman already running");
      clipman_common_show_info_dialog ();
      g_object_unref (app);
      return FALSE;
    }

  g_set_application_name (_("Clipman"));
  plugin = status_icon_register ();
  install_autostart_file ();

  g_signal_connect_swapped (app, "activate", G_CALLBACK (plugin_popup_menu), plugin);

  gtk_main ();

  g_object_unref (plugin->status_icon);
  g_object_unref (app);

  return FALSE;
}

/*
 * Status Icon
 */

static gboolean
cb_status_icon_is_embedded (gpointer user_data)
{
  GtkStatusIcon *status_icon = user_data;

  if (!gtk_status_icon_is_embedded (status_icon))
    {
      g_warning ("Status Icon is not embedded");
      gtk_main_quit ();
    }
  return FALSE;
}

static MyPlugin *
status_icon_register (void)
{
  MyPlugin *plugin = plugin_register ();
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();

  /* Menu Position Func */
  plugin->menu_position_func = (GtkMenuPositionFunc)gtk_status_icon_position_menu;

  /* Status Icon */
  if (gtk_icon_theme_has_icon (icon_theme, "clipman"))
    {
      plugin->status_icon = gtk_status_icon_new_from_icon_name ("clipman");
    }
  else
    {
      plugin->status_icon = gtk_status_icon_new_from_icon_name ("edit-paste");
    }
  //gtk_status_icon_set_tooltip (plugin->status_icon, _("Clipman"));
  g_timeout_add_seconds (60, cb_status_icon_is_embedded, plugin->status_icon);

  /* Signals */
  g_signal_connect_swapped (plugin->status_icon, "activate",
                            G_CALLBACK (cb_status_icon_activate), plugin);
  plugin->popup_menu_id =
    g_signal_connect_swapped (plugin->status_icon, "popup-menu",
                              G_CALLBACK (cb_status_icon_popup_menu), plugin);
  g_object_weak_ref (G_OBJECT (plugin->status_icon), cb_status_icon_finalize, plugin);

  return plugin;
}

static void
cb_status_icon_activate (MyPlugin *plugin)
{
  plugin_popup_menu (plugin);
}

static void
cb_status_icon_popup_menu (MyPlugin *plugin, guint button, guint activate_time)
{
  GtkWidget *mi;

  if (G_UNLIKELY (plugin->popup_menu == NULL))
    {
      plugin->popup_menu = gtk_menu_new ();
      mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      g_signal_connect_swapped (mi, "activate", G_CALLBACK (plugin_configure), plugin);
      mi = gtk_check_menu_item_new_with_mnemonic (_("_Disable"));
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      xfconf_g_property_bind (plugin->channel, "/tweaks/inhibit",
                              G_TYPE_BOOLEAN, mi, "active");
      mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      g_signal_connect_swapped (mi, "activate", G_CALLBACK (plugin_about), plugin);
      mi = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
      g_signal_connect_swapped (mi, "activate", G_CALLBACK (cb_status_icon_quit), plugin);
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      gtk_widget_show_all (plugin->popup_menu);
    }

  gtk_menu_set_screen (GTK_MENU (plugin->popup_menu), gtk_status_icon_get_screen (plugin->status_icon));

  if(!gtk_widget_has_grab(plugin->popup_menu))
  {
    gtk_grab_add(plugin->popup_menu);
  }

  gtk_menu_popup (GTK_MENU (plugin->popup_menu), NULL, NULL,
                  (GtkMenuPositionFunc)gtk_status_icon_position_menu, plugin->status_icon,
                  0, gtk_get_current_event_time ());
}

static void
cb_status_icon_quit (MyPlugin *plugin)
{
  update_autostart_file (FALSE);
  gtk_status_icon_set_visible (plugin->status_icon, FALSE);
  gtk_main_quit ();
}

static void
cb_status_icon_finalize (gpointer  data,
                         GObject  *where_the_object_was)
{
  MyPlugin *plugin = data;

  plugin_save (plugin);
  plugin_free (plugin);
}

static void
install_autostart_file (void)
{
  gchar *sysfile;
  gchar *userfile;
  GKeyFile *keyfile;
  gchar *data;

  sysfile = g_strdup (SYSCONFDIR"/xdg/autostart/"PACKAGE"-autostart.desktop");
  userfile = g_strdup_printf ("%s/autostart/"PACKAGE"-autostart.desktop", g_get_user_config_dir ());

  if (!g_file_test (sysfile, G_FILE_TEST_EXISTS))
    {
      g_warning ("The autostart file (%s) is missing in the system installation", sysfile);
      goto out;
    }

  /* Check if the user autostart file exists */
  if (g_file_test (userfile, G_FILE_TEST_EXISTS))
    {
      update_autostart_file (TRUE);
      goto out;
    }

  /* Copy the file */
  keyfile = g_key_file_new ();
  g_key_file_load_from_file (keyfile, sysfile, G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
  g_key_file_set_boolean (keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_HIDDEN, FALSE);
  data = g_key_file_to_data (keyfile, NULL, NULL);
  g_file_set_contents (userfile, data, -1, NULL);
  g_free (data);
  g_key_file_free (keyfile);

out:
  g_free (sysfile);
  g_free (userfile);
}

static void
update_autostart_file (gboolean autostart)
{
  gchar *userfile;
  GKeyFile *keyfile;
  gchar *data;

  userfile = g_strdup_printf ("%s/autostart/"PACKAGE"-autostart.desktop", g_get_user_config_dir ());

  /* Check if the user autostart file exists */
  if (!g_file_test (userfile, G_FILE_TEST_EXISTS))
    {
      g_warning ("The autostart file (%s) is not installed, what did you do?", userfile);
      g_free (userfile);
      return;
    }

  /* Update the file */
  keyfile = g_key_file_new ();
  g_key_file_load_from_file (keyfile, userfile, G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
  g_key_file_set_boolean (keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_HIDDEN, !autostart);
  g_key_file_set_string (keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TRY_EXEC, "xfce4-clipman");
  g_key_file_set_string (keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, "xfce4-clipman");
  data = g_key_file_to_data (keyfile, NULL, NULL);
  g_file_set_contents (userfile, data, -1, NULL);
  g_free (data);
  g_key_file_free (keyfile);
  g_free (userfile);
}
G_GNUC_END_IGNORE_DEPRECATIONS
