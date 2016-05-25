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

#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "common.h"

static gboolean
clipman_plugin_check_is_running (GtkWidget *widget,
                                 Window *xid)
{
  GdkScreen          *gscreen;
  gchar              *selection_name;
  Atom                selection_atom;
  Display            *display;

  display = gdk_x11_get_default_xdisplay ();
  gscreen = gtk_widget_get_screen (widget);
  selection_name = g_strdup_printf (XFCE_CLIPMAN_SELECTION"%d",
                                    gdk_screen_get_number (gscreen));
  selection_atom = XInternAtom (display, selection_name, FALSE);
  g_free(selection_name);

  if ((*xid = XGetSelectionOwner (display, selection_atom)))
    return TRUE;

  return FALSE;
}

gint
main (gint argc, gchar *argv[])
{
  XEvent                event;
  GtkWidget             *win;
  GdkWindow             *window;
  Window                id;
  Display               *display;

  gtk_init (&argc, &argv);

  win = gtk_invisible_new ();
  gtk_widget_realize (win);

  window = gtk_widget_get_window (GTK_WIDGET (win));
  display = gdk_x11_display_get_xdisplay (gdk_window_get_display (window));
  event.xclient.type = ClientMessage;
  event.xclient.message_type = XInternAtom (display, "STRING", False);
  event.xclient.format = 8;
  if (!g_ascii_strcasecmp(argv[0], "xfce4-popup-clipman-actions"))
    {
      g_snprintf (event.xclient.data.b, sizeof (event.xclient.data.b), XFCE_CLIPMAN_ACTION_MESSAGE);
    }
  else
    {
      g_snprintf (event.xclient.data.b, sizeof (event.xclient.data.b), XFCE_CLIPMAN_MESSAGE);
    }

  if (clipman_plugin_check_is_running (win, &id)) {
    event.xclient.window = id;
    XSendEvent (display,
                (Window) id,
                False,
                NoEventMask,
                &event);
    }
  else
    g_warning ("Can't find the xfce4-clipman-plugin.\n");
  gdk_flush ();

  gtk_widget_destroy (win);

  return FALSE;
}
