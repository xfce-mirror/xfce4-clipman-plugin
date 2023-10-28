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
#include <libxfce4panel/libxfce4panel.h>

#include "common.h"
#include "plugin.h"

/*
 * Panel Plugin
 */

static void             panel_plugin_register           (XfcePanelPlugin *panel_plugin);
static gboolean         panel_plugin_register_check     (GdkScreen *screen);
XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK (panel_plugin_register, panel_plugin_register_check);

static gboolean         plugin_set_size                 (MyPlugin *plugin,
                                                         gint size);
static gboolean         cb_button_pressed               (GtkButton *button,
                                                         GdkEventButton *event,
                                                         MyPlugin *plugin);
static void             cb_menu_deactivate              (GtkMenuShell *menu,
                                                         MyPlugin *plugin);
static void             cb_inhibit_toggled              (GtkCheckMenuItem *mi,
                                                         gpointer user_data);
static void             my_plugin_position_menu         (GtkMenu *menu,
                                                         gint *x,
                                                         gint *y,
                                                         gboolean *push_in,
                                                         MyPlugin *plugin);



static MyPlugin *my_plugin;

static gboolean
panel_plugin_register_check (GdkScreen *screen)
{
  my_plugin = plugin_register ();

  return my_plugin != NULL;
}

static void
panel_plugin_register (XfcePanelPlugin *panel_plugin)
{
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
  GtkWidget *mi = NULL;
  GtkCssProvider *css_provider;
  GtkStyleContext *context;

  /* Menu Position Func */
  my_plugin->menu_position_func = (GtkMenuPositionFunc)my_plugin_position_menu;

  /* Panel Plugin */
  my_plugin->panel_plugin = panel_plugin;
  gtk_widget_set_tooltip_text (GTK_WIDGET (panel_plugin), _("Clipman"));

  /* Panel Button */
  my_plugin->button = xfce_panel_create_toggle_button ();
  if (gtk_icon_theme_has_icon (icon_theme, "clipman-symbolic"))
      my_plugin->image = gtk_image_new_from_icon_name ("clipman-symbolic", GTK_ICON_SIZE_MENU);
  else if (gtk_icon_theme_has_icon (icon_theme, "edit-paste-symbolic"))
      my_plugin->image = gtk_image_new_from_icon_name ("edit-paste-symbolic", GTK_ICON_SIZE_MENU);
  else
      my_plugin->image = gtk_image_new_from_icon_name ("edit-paste", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (my_plugin->button), my_plugin->image);
  gtk_container_add (GTK_CONTAINER (panel_plugin), my_plugin->button);
  gtk_widget_set_name (GTK_WIDGET (my_plugin->button), "xfce4-clipman-plugin");

  /* Sane default Gtk style */
  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css_provider,
                                   ".inhibited { opacity: 0.5; }",
                                   -1, NULL);
  context = GTK_STYLE_CONTEXT (gtk_widget_get_style_context (GTK_WIDGET (my_plugin->image)));
  gtk_style_context_add_provider (context,
                                  GTK_STYLE_PROVIDER (css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css_provider);

  xfce_panel_plugin_set_small (panel_plugin, TRUE);

  xfce_panel_plugin_add_action_widget (panel_plugin, my_plugin->button);
  g_signal_connect (my_plugin->button, "button-press-event",
                    G_CALLBACK (cb_button_pressed), my_plugin);

  /* Context menu */
  xfce_panel_plugin_menu_show_about (panel_plugin);
  xfce_panel_plugin_menu_show_configure (panel_plugin);
  mi = gtk_check_menu_item_new_with_mnemonic (_("_Disable"));
  gtk_widget_show (mi);
  xfce_panel_plugin_menu_insert_item (panel_plugin, GTK_MENU_ITEM (mi));
  g_signal_connect (G_OBJECT (mi), "toggled",
                    G_CALLBACK (cb_inhibit_toggled), my_plugin->image);
  xfconf_g_property_bind (my_plugin->channel, "/tweaks/inhibit",
                          G_TYPE_BOOLEAN, mi, "active");

  /* Signals */
  g_signal_connect_swapped (panel_plugin, "about",
                            G_CALLBACK (plugin_about), my_plugin);
  g_signal_connect_swapped (panel_plugin, "configure-plugin",
                            G_CALLBACK (plugin_configure), my_plugin);
  g_signal_connect_swapped (panel_plugin, "save",
                            G_CALLBACK (plugin_save), my_plugin);
  g_signal_connect_swapped (panel_plugin, "free-data",
                            G_CALLBACK (plugin_free), my_plugin);
  g_signal_connect_swapped (panel_plugin, "size-changed",
                            G_CALLBACK (plugin_set_size), my_plugin);
  g_signal_connect (my_plugin->menu, "deactivate",
                    G_CALLBACK (cb_menu_deactivate), my_plugin);

  gtk_widget_show_all (GTK_WIDGET (panel_plugin));
}

static gboolean
plugin_set_size (MyPlugin *plugin,
                 gint size)
{
  gint icon_size;

  size /= xfce_panel_plugin_get_nrows (plugin->panel_plugin);
  gtk_widget_set_size_request (GTK_WIDGET (plugin->button), size, size);
  icon_size = xfce_panel_plugin_get_icon_size (XFCE_PANEL_PLUGIN (plugin->panel_plugin));
  gtk_image_set_pixel_size (GTK_IMAGE (plugin->image), icon_size);

  return TRUE;
}

static gboolean
cb_button_pressed (GtkButton *button,
                   GdkEventButton *event,
                   MyPlugin *plugin)
{
  gboolean inhibit;

  if (event->button != 1 && event->button != 2 && !(event->state & GDK_CONTROL_MASK))
    return FALSE;
  else if (event->button == 2)
    {
      inhibit = xfconf_channel_get_bool (plugin->channel, "/tweaks/inhibit", FALSE);
      xfconf_channel_set_bool (plugin->channel, "/tweaks/inhibit", !inhibit);
    }
  else if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
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
cb_inhibit_toggled (GtkCheckMenuItem *mi,
                    gpointer user_data)
{
  GtkStyleContext *context;
  GtkWidget *image = GTK_WIDGET (user_data);

  g_return_if_fail (GTK_IS_WIDGET (image));

  context = gtk_widget_get_style_context (GTK_WIDGET (image));

  if (gtk_check_menu_item_get_active (mi))
    gtk_style_context_add_class (context, "inhibited");
  else
    gtk_style_context_remove_class (context, "inhibited");
}

static void
my_plugin_position_menu (GtkMenu *menu,
                         gint *x,
                         gint *y,
                         gboolean *push_in,
                         MyPlugin *plugin)
{
  gboolean above = TRUE;
  gint button_width, button_height;
  GtkRequisition minimum_size;
  GtkRequisition natural_size;
  XfceScreenPosition screen_position;
  GdkRectangle *geometry;

  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin->panel_plugin));

  screen_position = xfce_panel_plugin_get_screen_position (plugin->panel_plugin);
  gtk_widget_get_size_request (plugin->button, &button_width, &button_height);
  gtk_widget_get_preferred_size (GTK_WIDGET (menu), &minimum_size, &natural_size);
  gdk_window_get_origin (gtk_widget_get_window (GTK_WIDGET (plugin->panel_plugin)), x, y);
  geometry = xfce_gdk_screen_get_geometry ();

  switch (screen_position)
    {
      case XFCE_SCREEN_POSITION_NW_H:
      case XFCE_SCREEN_POSITION_N:
      case XFCE_SCREEN_POSITION_NE_H:
        above = FALSE;
        G_GNUC_FALLTHROUGH;
      case XFCE_SCREEN_POSITION_SW_H:
      case XFCE_SCREEN_POSITION_S:
      case XFCE_SCREEN_POSITION_SE_H:
        if (above)
          /* Show menu above */
          *y -= minimum_size.height;
        else
          /* Show menu below */
          *y += button_height;

        if (*x + minimum_size.width > geometry->width)
          /* Adjust horizontal position */
          *x = geometry->width - minimum_size.width;

        break;

      default:
        if (*x + button_width + minimum_size.width > geometry->width)
          /* Show menu on the right */
          *x -= minimum_size.width;
        else
          /* Show menu on the left */
          *x += button_width;

        if (*y + minimum_size.height > geometry->height)
          /* Adjust vertical position */
          *y = geometry->height - minimum_size.height;

        break;
    }
}
