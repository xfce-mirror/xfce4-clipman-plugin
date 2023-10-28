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
#include <libxfce4util/libxfce4util.h>

#include <common.h>



static void
grab_keyboard (void)
{
  GtkWidget *invisible = gtk_invisible_new ();
  GdkSeat *seat = gdk_display_get_default_seat (gdk_display_get_default ());
  GdkGrabStatus grab_status = GDK_GRAB_FAILED;

  gtk_widget_show (invisible);
  for (gint i = 0; i < 2500 && grab_status != GDK_GRAB_SUCCESS; i++)
    {
      grab_status = gdk_seat_grab (seat, gtk_widget_get_window (invisible),
                                   GDK_SEAT_CAPABILITY_KEYBOARD, TRUE,
                                   NULL, NULL, NULL, NULL);
      g_usleep (1000);
    }
  gtk_widget_destroy (invisible);

  if (grab_status == GDK_GRAB_SUCCESS)
    gdk_seat_ungrab (seat);
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

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  gtk_init (&argc, &argv);

  app = gtk_application_new ("org.xfce.clipman", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "handle-local-options", G_CALLBACK (handle_local_options), NULL);
  ret = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return ret;
}
