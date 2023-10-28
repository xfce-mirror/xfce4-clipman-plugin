/*
 * Copyright (C) 2023 Gaël Bonithon <gael@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#include <clipboard-manager/clipboard-manager-x11.h>
#endif
#ifdef ENABLE_WAYLAND
#include <gdk/gdkwayland.h>
#include <clipboard-manager/clipboard-manager-wayland.h>
#endif

#include <clipboard-manager/clipboard-manager.h>



G_DEFINE_INTERFACE (XcpClipboardManager, xcp_clipboard_manager, G_TYPE_OBJECT)



static void
xcp_clipboard_manager_default_init (XcpClipboardManagerInterface *iface)
{
}



XcpClipboardManager *
xcp_clipboard_manager_get (void)
{
 static XcpClipboardManager *manager = NULL;

  if (manager != NULL)
    return g_object_ref (manager);

#ifdef ENABLE_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    manager = g_object_new (XCP_TYPE_CLIPBOARD_MANAGER_X11, NULL);
#endif
#ifdef ENABLE_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ()))
    manager = g_object_new (XCP_TYPE_CLIPBOARD_MANAGER_WAYLAND, NULL);
#endif

  if (manager != NULL)
    g_object_add_weak_pointer (G_OBJECT (manager), (gpointer *) &manager);
  else
    g_warning ("Clipboard manager is not supported on this windowing environment");

  return manager;
}
