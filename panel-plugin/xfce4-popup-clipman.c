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


/* Initial code was taken from xfwm4/src/menu.c:grab_available().
 * TODO replace deprecated GTK/GDK functions.
 */
static gboolean
grab_keyboard ()
{
  guint32 timestamp = GDK_CURRENT_TIME;
  GdkScreen *screen = gdk_screen_get_default ();
  GdkWindow *win = gdk_screen_get_root_window (screen);
  GdkEventMask mask =
    GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
    GDK_POINTER_MOTION_MASK;
  GdkGrabStatus grab_status;
  gboolean grab_failed = FALSE;
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

gint
main (gint argc, gchar *argv[])
{
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
      grab_keyboard ();
      g_application_activate (G_APPLICATION (app));
      g_object_unref (app);
    }
  else
    {
      g_warning ("Unable to find the primary instance org.xfce.clipman");
    }

  return FALSE;
}
