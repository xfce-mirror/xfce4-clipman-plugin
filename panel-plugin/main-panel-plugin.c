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
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>

#include "common.h"
#include "plugin.h"

/*
 * Panel Plugin
 */

static void             panel_plugin_register           (XfcePanelPlugin *panel_plugin);
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (panel_plugin_register);

static gboolean         panel_plugin_set_size           (MyPlugin *plugin,
                                                         gint size);
static gboolean         cb_button_pressed               (GtkButton *button,
                                                         GdkEventButton *event,
                                                         MyPlugin *plugin);
static void             cb_menu_deactivate              (GtkMenuShell *menu,
                                                         MyPlugin *plugin);
static void             my_plugin_position_menu         (GtkMenu *menu,
                                                         gint *x,
                                                         gint *y,
                                                         gboolean *push_in,
                                                         MyPlugin *plugin);



/*
 * Panel Plugin
 */

static void
panel_plugin_register (XfcePanelPlugin *panel_plugin)
{
  MyPlugin *plugin = plugin_register ();

  /* Menu Position Func */
  plugin->menu_position_func = (GtkMenuPositionFunc)my_plugin_position_menu;

  /* Panel Plugin */
  plugin->panel_plugin = panel_plugin;
#if GTK_CHECK_VERSION (2,12,0)
  gtk_widget_set_tooltip_text (GTK_WIDGET (panel_plugin), _("Clipman"));
#endif

  /* Panel Button */
  plugin->button = xfce_create_panel_toggle_button ();
  /* The image is set through the set_size callback */
  plugin->image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->image);
  gtk_container_add (GTK_CONTAINER (panel_plugin), plugin->button);
  xfce_panel_plugin_add_action_widget (panel_plugin, plugin->button);
  g_signal_connect (plugin->button, "button-press-event",
                    G_CALLBACK (cb_button_pressed), plugin);

  /* Signals */
  g_signal_connect_swapped (panel_plugin, "size-changed",
                            G_CALLBACK (panel_plugin_set_size), plugin);
  xfce_panel_plugin_menu_show_about (panel_plugin);
  g_signal_connect_swapped (panel_plugin, "about",
                            G_CALLBACK (plugin_about), plugin);
  xfce_panel_plugin_menu_show_configure (panel_plugin);
  g_signal_connect_swapped (panel_plugin, "configure-plugin",
                            G_CALLBACK (plugin_configure), plugin);
  g_signal_connect_swapped (panel_plugin, "save",
                            G_CALLBACK (plugin_save), plugin);
  g_signal_connect_swapped (panel_plugin, "free-data",
                            G_CALLBACK (plugin_free), plugin);
  g_signal_connect (plugin->menu, "deactivate",
                    G_CALLBACK (cb_menu_deactivate), plugin);

  gtk_widget_show_all (GTK_WIDGET (panel_plugin));
}

static gboolean
panel_plugin_set_size (MyPlugin *plugin,
                       gint size)
{
  GdkPixbuf *pixbuf;

  gtk_widget_set_size_request (plugin->button, size, size);

  size -= 2 + 2 * MAX (plugin->button->style->xthickness,
                       plugin->button->style->ythickness);
  pixbuf = xfce_themed_icon_load (GTK_STOCK_PASTE, size);
  gtk_image_set_from_pixbuf (GTK_IMAGE (plugin->image), pixbuf);
  g_object_unref (G_OBJECT (pixbuf));

  return TRUE;
}

static gboolean
cb_button_pressed (GtkButton *button,
                   GdkEventButton *event,
                   MyPlugin *plugin)
{
  if (event->button != 1 && !(event->state & GDK_CONTROL_MASK))
    return FALSE;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    plugin_popup_menu (plugin);

  return TRUE;
}

static void
cb_menu_deactivate (GtkMenuShell *menu,
                    MyPlugin *plugin)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), FALSE);
}

static void
my_plugin_position_menu (GtkMenu *menu,
                         gint *x,
                         gint *y,
                         gboolean *push_in,
                         MyPlugin *plugin)
{
  GtkWidget *button;
  GtkRequisition requisition;
  GtkOrientation orientation;

  button = plugin->button;
  orientation = xfce_panel_plugin_get_orientation (plugin->panel_plugin);
  gtk_widget_size_request (GTK_WIDGET (menu), &requisition);
  gdk_window_get_origin (GTK_WIDGET (plugin->panel_plugin)->window, x, y);

  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      if (*y + button->allocation.height + requisition.height > gdk_screen_height ())
        /* Show menu above */
        *y -= requisition.height;
      else
        /* Show menu below */
        *y += button->allocation.height;

      if (*x + requisition.width > gdk_screen_width ())
        /* Adjust horizontal position */
        *x = gdk_screen_width () - requisition.width;
      break;

    case GTK_ORIENTATION_VERTICAL:
      if (*x + button->allocation.width + requisition.width > gdk_screen_width ())
        /* Show menu on the right */
        *x -= requisition.width;
      else
        /* Show menu on the left */
        *x += button->allocation.width;

      if (*y + requisition.height > gdk_screen_height ())
        /* Adjust vertical position */
        *y = gdk_screen_height () - requisition.height;
      break;

    default:
      break;
    }
}

