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
      /* FIXME g_usleep is a workaround when using the popup command through a
       * keyboard shortcut, in fact the code needs to call gdk_seat_grb/ungrab
       * for the gtkmenu to show up.
       */
      g_usleep(500000);
      g_application_activate (G_APPLICATION (app));
      g_object_unref (app);
      return 0;
    }
  else
    {
      g_warning ("Unable to find the primary instance org.xfce.clipman");
    }

  return FALSE;
}
