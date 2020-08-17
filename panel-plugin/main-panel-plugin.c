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
XFCE_PANEL_PLUGIN_REGISTER (panel_plugin_register);

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



/*
 * Panel Plugin
 */

static void
panel_plugin_register (XfcePanelPlugin *panel_plugin)
{
  MyPlugin *plugin = plugin_register ();
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
  GtkWidget *mi = NULL;
  GtkCssProvider *css_provider;
  GtkStyleContext *context;

  /* Menu Position Func */
  plugin->menu_position_func = (GtkMenuPositionFunc)my_plugin_position_menu;

  /* Panel Plugin */
  plugin->panel_plugin = panel_plugin;
  gtk_widget_set_tooltip_text (GTK_WIDGET (panel_plugin), _("Clipman"));

  /* Panel Button */
  plugin->button = xfce_panel_create_toggle_button ();
  if (gtk_icon_theme_has_icon (icon_theme, "clipman-symbolic"))
      plugin->image = gtk_image_new_from_icon_name ("clipman-symbolic", GTK_ICON_SIZE_MENU);
  else if (gtk_icon_theme_has_icon (icon_theme, "edit-paste-symbolic"))
      plugin->image = gtk_image_new_from_icon_name ("edit-paste-symbolic", GTK_ICON_SIZE_MENU);
  else
      plugin->image = gtk_image_new_from_icon_name ("edit-paste", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->image);
  gtk_container_add (GTK_CONTAINER (panel_plugin), plugin->button);
  gtk_widget_set_name (GTK_WIDGET (plugin->button), "xfce4-clipman-plugin");

  /* Sane default Gtk style */
  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css_provider,
                                   ".inhibited { opacity: 0.5; }",
                                   -1, NULL);
  context = GTK_STYLE_CONTEXT (gtk_widget_get_style_context (GTK_WIDGET (plugin->image)));
  gtk_style_context_add_provider (context,
                                  GTK_STYLE_PROVIDER (css_provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css_provider);

  xfce_panel_plugin_set_small (panel_plugin, TRUE);

  xfce_panel_plugin_add_action_widget (panel_plugin, plugin->button);
  g_signal_connect (plugin->button, "button-press-event",
                    G_CALLBACK (cb_button_pressed), plugin);

  /* Context menu */
  xfce_panel_plugin_menu_show_about (panel_plugin);
  xfce_panel_plugin_menu_show_configure (panel_plugin);
  mi = gtk_check_menu_item_new_with_mnemonic (_("_Disable"));
  gtk_widget_show (mi);
  xfce_panel_plugin_menu_insert_item (panel_plugin, GTK_MENU_ITEM (mi));
  g_signal_connect (G_OBJECT (mi), "toggled",
                    G_CALLBACK (cb_inhibit_toggled), plugin->image);
  xfconf_g_property_bind (plugin->channel, "/tweaks/inhibit",
                          G_TYPE_BOOLEAN, mi, "active");

  /* Signals */
  g_signal_connect_swapped (panel_plugin, "about",
                            G_CALLBACK (plugin_about), plugin);
  g_signal_connect_swapped (panel_plugin, "configure-plugin",
                            G_CALLBACK (plugin_configure), plugin);
  g_signal_connect_swapped (panel_plugin, "save",
                            G_CALLBACK (plugin_save), plugin);
  g_signal_connect_swapped (panel_plugin, "free-data",
                            G_CALLBACK (plugin_free), plugin);
  g_signal_connect_swapped (panel_plugin, "size-changed",
                            G_CALLBACK (plugin_set_size), plugin);
  g_signal_connect (plugin->menu, "deactivate",
                    G_CALLBACK (cb_menu_deactivate), plugin);

  gtk_widget_show_all (GTK_WIDGET (panel_plugin));
}

static gboolean
plugin_set_size (MyPlugin *plugin,
                 gint size)
{
#if !LIBXFCE4PANEL_CHECK_VERSION (4,13,0)
  GtkStyleContext *context;
  GtkBorder padding, border;
  gint width;
  gint xthickness;
  gint ythickness;
#endif
  gint icon_size;

  size /= xfce_panel_plugin_get_nrows (plugin->panel_plugin);
  gtk_widget_set_size_request (GTK_WIDGET (plugin->button), size, size);
#if LIBXFCE4PANEL_CHECK_VERSION (4,13,0)
  icon_size = xfce_panel_plugin_get_icon_size (XFCE_PANEL_PLUGIN (plugin->panel_plugin));
#else
  /* Calculate the size of the widget because the theme can override it */
  context = gtk_widget_get_style_context (GTK_WIDGET (plugin->button));
  gtk_style_context_get_padding (context, gtk_widget_get_state_flags (GTK_WIDGET (plugin->button)), &padding);
  gtk_style_context_get_border (context, gtk_widget_get_state_flags (GTK_WIDGET (plugin->button)), &border);
  xthickness = padding.left + padding.right + border.left + border.right;
  ythickness = padding.top + padding.bottom + border.top + border.bottom;

  /* Calculate the size of the space left for the icon */
  width = size - 2 * MAX (xthickness, ythickness);

  /* Since symbolic icons are usually only provided in 16px we
   * try to be clever and use size steps */
  if (width <= 21)
    icon_size = 16;
  else if (width >=22 && width <= 29)
    icon_size = 24;
  else if (width >= 30 && width <= 40)
    icon_size = 32;
  else
    icon_size = width;
#endif

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
