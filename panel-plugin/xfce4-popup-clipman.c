/*
 *  Clipman - panel plugin for Xfce Desktop Environment
 *            popup command to show the clipman menu
 *  Copyright (C) 2002-2006  Olivier Fourdan
 *                2008-2012  Mike Massonnet <mmassonnet@xfce.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <common.h>


/* Initial code was taken from xfwm4/src/menu.c:grab_available().
 * TODO replace deprecated GTK/GDK functions.
 */
static void
grab_keyboard (void)
{
  guint32 timestamp = GDK_CURRENT_TIME;
  GdkScreen *screen = gdk_screen_get_default ();
  GdkWindow *win = gdk_screen_get_root_window (screen);
  GdkGrabStatus grab_status;
  gint i = 0;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  grab_status = gdk_keyboard_grab (win, TRUE, timestamp);
G_GNUC_END_IGNORE_DEPRECATIONS

  while ((i++ < 2500) && (grab_status != GDK_GRAB_SUCCESS))
    {
      g_usleep (1000);
      if (grab_status != GDK_GRAB_SUCCESS)
        {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          grab_status = gdk_keyboard_grab (win, TRUE, timestamp);
G_GNUC_END_IGNORE_DEPRECATIONS
        }
    }

  if (grab_status == GDK_GRAB_SUCCESS)
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      gdk_keyboard_ungrab (timestamp);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
}

static gint
handle_local_options (GApplication *app,
                      GVariantDict *options,
                      gpointer user_data)
{
  GError *error = NULL;

  if (!g_application_register (app, NULL, &error))
    {
      g_warning ("Unable to register GApplication: %s", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  if (!g_application_get_is_remote (app))
    {
      g_warning ("Unable to find the primary instance org.xfce.clipman");
      clipman_common_show_warning_dialog ();
      return EXIT_FAILURE;
    }

  /* ensure grab is available when popup command is activated via keyboard shortcut */
  grab_keyboard ();

  /* activate primary instance */
  return -1;
}

gint
main (gint argc, gchar *argv[])
{
  GtkApplication *app;
  gint ret;

  gtk_init (&argc, &argv);

  app = gtk_application_new ("org.xfce.clipman", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "handle-local-options", G_CALLBACK (handle_local_options), NULL);
  ret = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return ret;
}
