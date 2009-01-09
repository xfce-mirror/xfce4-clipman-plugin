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

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <xfconf/xfconf.h>

#include "collector.h"
#include "history.h"
#include "menu.h"



/*
 * MyPlugin structure
 */

typedef struct _MyPlugin MyPlugin;
struct _MyPlugin
{
  XfcePanelPlugin      *panel_plugin;
  XfconfChannel        *channel;
  ClipmanCollector     *collector;
  ClipmanHistory       *history;
  GtkWidget            *button;
  GtkWidget            *image;
  GtkWidget            *menu;
};

/*
 * Panel Plugin registration
 */

static void panel_plugin_register (XfcePanelPlugin *panel_plugin);
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (panel_plugin_register);

/*
 * Panel Plugin functions declarations
 */

static gboolean         panel_plugin_set_size           (XfcePanelPlugin *panel_plugin,
                                                         int size,
                                                         MyPlugin *plugin);
static void             panel_plugin_free               (XfcePanelPlugin *panel_plugin,
                                                         MyPlugin *plugin);
static void             cb_button_toggled               (GtkToggleButton *button,
                                                         MyPlugin *plugin);
static void             cb_menu_deactivate              (GtkMenuShell *menu,
                                                         MyPlugin *plugin);
static void             my_plugin_position_menu         (GtkMenu *menu,
                                                         gint *x,
                                                         gint *y,
                                                         gboolean *push_in,
                                                         MyPlugin *plugin);



/*
 * Panel Plugin functions
 */

static void
panel_plugin_register (XfcePanelPlugin *panel_plugin)
{
  MyPlugin *plugin;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  xfconf_init (NULL);

  /* XfcePanelPlugin widget */
#if GTK_CHECK_VERSION (2,12,0)
  gtk_widget_set_tooltip_text (GTK_WIDGET (panel_plugin), _("Clipman"));
#endif

  /* MyPlugin */
  plugin = g_slice_new0 (MyPlugin);

  /* Keep a pointer on the panel plugin */
  plugin->panel_plugin = panel_plugin;

  /* XfconfChannel */
  plugin->channel = xfconf_channel_new_with_property_base ("xfce4-panel", "/plugins/clipman");

  /* ClipmanHistory */
  plugin->history = clipman_history_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/max-texts-in-history",
                          G_TYPE_UINT, plugin->history, "max-texts-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/max-images-in-history",
                          G_TYPE_UINT, plugin->history, "max-images-in-history");

  /* ClipmanCollector */
  plugin->collector = clipman_collector_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/ignore-primary-clipboard",
                          G_TYPE_BOOLEAN, plugin->collector, "ignore-primary-clipboard");

  /* Panel Button */
  plugin->button = xfce_create_panel_toggle_button ();
  /* The image is set through the set_size callback */
  plugin->image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->image);
  gtk_container_add (GTK_CONTAINER (panel_plugin), plugin->button);
  xfce_panel_plugin_add_action_widget (panel_plugin, plugin->button);
  g_signal_connect (plugin->button, "toggled",
                    G_CALLBACK (cb_button_toggled), plugin);

  /* ClipmanMenu */
  plugin->menu = clipman_menu_new ();
  g_signal_connect (plugin->menu, "deactivate",
                    G_CALLBACK (cb_menu_deactivate), plugin);

  /* Panel Plugin Signals */
  g_signal_connect (panel_plugin, "size-changed",
                    G_CALLBACK (panel_plugin_set_size), plugin);
  g_signal_connect (panel_plugin, "free-data",
                    G_CALLBACK (panel_plugin_free), plugin);

  gtk_widget_show_all (GTK_WIDGET (panel_plugin));
}

static gboolean
panel_plugin_set_size (XfcePanelPlugin *panel_plugin,
                       int size,
                       MyPlugin *plugin)
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

static void
panel_plugin_free (XfcePanelPlugin *panel_plugin,
                   MyPlugin *plugin)
{
  gtk_widget_destroy (plugin->menu);
  gtk_widget_destroy (plugin->button);
  g_object_unref (plugin->channel);
  g_object_unref (plugin->collector);
  g_object_unref (plugin->history);
  g_slice_free (MyPlugin, plugin);
  xfconf_shutdown ();
}

static void
cb_button_toggled (GtkToggleButton *button,
                   MyPlugin *plugin)
{
  if (gtk_toggle_button_get_active (button))
    {
      gtk_menu_set_screen (GTK_MENU (plugin->menu), gtk_widget_get_screen (plugin->button));
      xfce_panel_plugin_register_menu (plugin->panel_plugin, GTK_MENU (plugin->menu));
      gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                      (GtkMenuPositionFunc)my_plugin_position_menu, plugin,
                      0, gtk_get_current_event_time ());
    }
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
  gdk_window_get_origin (button->window, x, y);

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

