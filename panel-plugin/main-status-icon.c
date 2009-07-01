/*
 *  Copyright (c) 2008-2009 Mike Massonnet <mmassonnet@xfce.org>
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
#include <libxfcegui4/libxfcegui4.h>

#include "common.h"
#include "plugin.h"

/*
 * Status Icon
 */

static MyPlugin*        status_icon_register            ();
static gboolean         cb_status_icon_is_embedded      (GtkStatusIcon *status_icon);
static void             cb_status_icon_activate         (MyPlugin *plugin);
static void             cb_status_icon_popup_menu       (MyPlugin *plugin,
                                                         guint button,
                                                         guint activate_time);
static void             cb_status_icon_quit             (MyPlugin *plugin);
static gboolean         cb_status_icon_set_size         (MyPlugin *plugin,
                                                         gint size);
static void             cb_status_icon_finalize         (MyPlugin *plugin);
static void             install_autostart_file          ();
static void             update_autostart_file           ();

/*
 * Plugin Registration
 */

gint
main (gint argc,
      gchar *argv[])
{
  MyPlugin *plugin;

  gtk_init (&argc, &argv);

  g_set_application_name (_("Clipman"));
  plugin = status_icon_register ();
  install_autostart_file ();

  gtk_main ();

  g_object_unref (plugin->status_icon);

  return 0;
}

/*
 * Status Icon
 */

static MyPlugin *
status_icon_register ()
{
  MyPlugin *plugin = plugin_register ();

  /* Status Icon */
  plugin->status_icon = gtk_status_icon_new ();
  gtk_status_icon_set_tooltip (plugin->status_icon, _("Clipman"));
  g_timeout_add_seconds (60, (GSourceFunc)cb_status_icon_is_embedded, plugin->status_icon);

  /* Signals */
  g_signal_connect_swapped (plugin->status_icon, "activate",
                            G_CALLBACK (cb_status_icon_activate), plugin);
  plugin->popup_menu_id =
    g_signal_connect_swapped (plugin->status_icon, "popup-menu",
                              G_CALLBACK (cb_status_icon_popup_menu), plugin);
  g_signal_connect_swapped (plugin->status_icon, "size-changed",
                            G_CALLBACK (cb_status_icon_set_size), plugin);
  g_object_weak_ref (G_OBJECT (plugin->status_icon), (GWeakNotify)cb_status_icon_finalize, plugin);

  return plugin;
}

static gboolean
cb_status_icon_is_embedded (GtkStatusIcon *status_icon)
{
  if (!gtk_status_icon_is_embedded (status_icon))
    {
      g_warning ("Status Icon is not embedded");
      gtk_main_quit ();
    }
  return FALSE;
}

static void
cb_status_icon_activate (MyPlugin *plugin)
{
  gtk_menu_set_screen (GTK_MENU (plugin->menu), gtk_status_icon_get_screen (plugin->status_icon));
  gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                  (GtkMenuPositionFunc)gtk_status_icon_position_menu, plugin->status_icon,
                  0, gtk_get_current_event_time ());
}

static void
cb_status_icon_popup_menu (MyPlugin *plugin, guint button, guint activate_time)
{
  GtkWidget *mi;

  if (G_UNLIKELY (plugin->popup_menu == NULL))
    {
      plugin->popup_menu = gtk_menu_new ();
      mi = gtk_menu_item_new_with_label (_("Clipman"));
      gtk_widget_set_sensitive (mi, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      mi = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      g_signal_connect_swapped (mi, "activate", G_CALLBACK (plugin_configure), plugin);
      mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      g_signal_connect_swapped (mi, "activate", G_CALLBACK (plugin_about), plugin);
      mi = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
      g_signal_connect_swapped (mi, "activate", G_CALLBACK (cb_status_icon_quit), plugin);
      gtk_menu_shell_append (GTK_MENU_SHELL (plugin->popup_menu), mi);
      gtk_widget_show_all (plugin->popup_menu);
    }

  gtk_menu_set_screen (GTK_MENU (plugin->popup_menu), gtk_status_icon_get_screen (plugin->status_icon));
  gtk_menu_popup (GTK_MENU (plugin->popup_menu), NULL, NULL,
                  (GtkMenuPositionFunc)NULL, NULL,
                  0, gtk_get_current_event_time ());
}

static void
cb_status_icon_quit (MyPlugin *plugin)
{
  gint res;
  res = xfce_message_dialog (NULL, _("Autostart Clipman"), GTK_STOCK_DIALOG_QUESTION,
                             _("Autostart Clipman"), _("Do you want to restart the clipboard manager the next time you login?"),
                             GTK_STOCK_YES, GTK_RESPONSE_YES, GTK_STOCK_NO, GTK_RESPONSE_NO, NULL);
  update_autostart_file (res == GTK_RESPONSE_YES);

  gtk_status_icon_set_visible (plugin->status_icon, FALSE);
  gtk_main_quit ();
}

static gboolean
cb_status_icon_set_size (MyPlugin *plugin, gint size)
{
  GdkPixbuf *pixbuf;
 
  pixbuf = xfce_themed_icon_load (GTK_STOCK_PASTE, size);
  gtk_status_icon_set_from_pixbuf (plugin->status_icon, pixbuf);
  g_object_unref (G_OBJECT (pixbuf));

  return TRUE;
}

static void
cb_status_icon_finalize (MyPlugin *plugin)
{
  plugin_save (plugin);
  plugin_free (plugin);
}

static void
install_autostart_file ()
{
  gchar *sysfile;
  gchar *userfile;
  gint res;
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
      goto out;
    }

  /* Ask the user */
  res = xfce_message_dialog (NULL, _("Autostart Clipman"), GTK_STOCK_DIALOG_QUESTION,
                             _("Autostart Clipman"), _("Do you want to autostart the clipboard manager?"),
                             GTK_STOCK_YES, GTK_RESPONSE_YES, GTK_STOCK_NO, GTK_RESPONSE_NO, NULL);

  /* Copy the file */
  keyfile = g_key_file_new ();
  g_key_file_load_from_file (keyfile, sysfile, G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
  if (res == GTK_RESPONSE_YES)
    {
      g_key_file_set_boolean (keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_HIDDEN, FALSE);
    }
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
  gint res;
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
  data = g_key_file_to_data (keyfile, NULL, NULL);
  g_file_set_contents (userfile, data, -1, NULL);
  g_free (data);
  g_key_file_free (keyfile);
  g_free (userfile);
}

