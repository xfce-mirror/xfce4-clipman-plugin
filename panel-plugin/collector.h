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

#ifndef __CLIPMAN_COLLECTOR_H__
#define __CLIPMAN_COLLECTOR_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define CLIPMAN_TYPE_COLLECTOR (clipman_collector_get_type ())
G_DECLARE_FINAL_TYPE (ClipmanCollector, clipman_collector, CLIPMAN, COLLECTOR, GObject)

ClipmanCollector *
clipman_collector_get (void);
void
clipman_collector_set_is_restoring (ClipmanCollector *collector,
                                    GtkClipboard *clipboard);
void
clipman_collector_clear_cache (ClipmanCollector *collector);
GdkPixbuf *
clipman_collector_get_current_image (ClipmanCollector *collector);

#endif /* !__CLIPMAN_COLLECTOR_H__ */
