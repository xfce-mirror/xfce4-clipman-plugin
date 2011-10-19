/*
 *  Copyright (c) 2008-2011 Mike Massonnet <mmassonnet@xfce.org>
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
#include <libxfce4panel/libxfce4panel.h>

#include "common.h"
#include "plugin.h"

/*
 * Panel Plugin
 */

static void             panel_plugin_register           (XfcePanelPlugin *panel_plugin);
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (panel_plugin_register);

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
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();

  /* Menu Position Func */
  plugin->menu_position_func = (GtkMenuPositionFunc)my_plugin_position_menu;

  /* Panel Plugin */
  plugin->panel_plugin = panel_plugin;
  gtk_widget_set_tooltip_text (GTK_WIDGET (panel_plugin), _("Clipman"));

  /* Panel Button */
  plugin->button = xfce_create_panel_toggle_button ();
  if (gtk_icon_theme_has_icon (icon_theme, "clipman"))
    {
      plugin->image = xfce_panel_image_new_from_source ("clipman");
    }
  else
    {
      plugin->image = xfce_panel_image_new_from_source (GTK_STOCK_PASTE);
    }
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->image);
  gtk_container_add (GTK_CONTAINER (panel_plugin), plugin->button);
  xfce_panel_plugin_add_action_widget (panel_plugin, plugin->button);
  g_signal_connect (plugin->button, "button-press-event",
                    G_CALLBACK (cb_button_pressed), plugin);

  /* Signals */
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
  gint button_width, button_height;
  GtkRequisition requisition;
  GtkOrientation orientation;

  button = plugin->button;
  orientation = xfce_panel_plugin_get_orientation (plugin->panel_plugin);
  gtk_widget_get_size_request (button, &button_width, &button_height);
  gtk_widget_size_request (GTK_WIDGET (menu), &requisition);
  gdk_window_get_origin (gtk_widget_get_window (GTK_WIDGET (plugin->panel_plugin)), x, y);

  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      if (*y + button_height + requisition.height > gdk_screen_height ())
        /* Show menu above */
        *y -= requisition.height;
      else
        /* Show menu below */
        *y += button_height;

      if (*x + requisition.width > gdk_screen_width ())
        /* Adjust horizontal position */
        *x = gdk_screen_width () - requisition.width;
      break;

    case GTK_ORIENTATION_VERTICAL:
      if (*x + button_width + requisition.width > gdk_screen_width ())
        /* Show menu on the right */
        *x -= requisition.width;
      else
        /* Show menu on the left */
        *x += button_width;

      if (*y + requisition.height > gdk_screen_height ())
        /* Adjust vertical position */
        *y = gdk_screen_height () - requisition.height;
      break;

    default:
      break;
    }
}

