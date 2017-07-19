/*
 *  Copyright (c) 2009-2012 Mike Massonnet <mmassonnet@xfce.org>
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

#ifndef __CLIPMAN_PLUGIN_H__
#define __CLIPMAN_PLUGIN_H__

#if ! defined (PANEL_PLUGIN) && ! defined (STATUS_ICON)
#error "None of PANEL_PLUGIN or STATUS_ICON is defined for preprocessing"
#endif

#include <gtk/gtk.h>
#include <xfconf/xfconf.h>

#ifdef PANEL_PLUGIN
#include <libxfce4panel/libxfce4panel.h>
#endif

#include <x11-clipboard-manager/daemon.h>
#include "actions.h"
#include "collector.h"
#include "history.h"

/*
 * MyPlugin structure
 */

typedef struct _MyPlugin MyPlugin;
struct _MyPlugin
{
#ifdef PANEL_PLUGIN
  XfcePanelPlugin      *panel_plugin;
  GtkWidget            *button;
  GtkWidget            *image;
#elif defined (STATUS_ICON)
  GtkStatusIcon        *status_icon;
#endif
  GsdClipboardManager  *daemon;
  XfconfChannel        *channel;
  ClipmanActions       *actions;
  ClipmanCollector     *collector;
  ClipmanHistory       *history;
  GtkWidget            *menu;
  GtkMenuPositionFunc   menu_position_func;
  GtkWidget            *popup_menu;
  gulong                popup_menu_id;
  GtkApplication       *app;
};

/*
 * Plugin functions
 */

MyPlugin*               plugin_register                 ();
void                    plugin_load                     (MyPlugin *plugin);
void                    plugin_save                     (MyPlugin *plugin);
void                    plugin_free                     (MyPlugin *plugin);
void                    plugin_about                    (MyPlugin *plugin);
void                    plugin_configure                (MyPlugin *plugin);
void                    plugin_popup_menu               (MyPlugin *plugin);

#endif /* !__CLIPMAN_PLUGIN_H__ */
