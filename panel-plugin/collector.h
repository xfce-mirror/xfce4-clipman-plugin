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

#ifndef __CLIPMAN_COLLECTOR_H__
#define __CLIPMAN_COLLECTOR_H__

#include <glib-object.h>

#define CLIPMAN_TYPE_COLLECTOR                  (clipman_collector_get_type())

#define CLIPMAN_COLLECTOR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLIPMAN_TYPE_COLLECTOR, ClipmanCollector))
#define CLIPMAN_COLLECTOR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CLIPMAN_TYPE_COLLECTOR, ClipmanCollectorClass))

#define CLIPMAN_IS_COLLECTOR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLIPMAN_TYPE_COLLECTOR))
#define CLIPMAN_IS_COLLECTOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CLIPMAN_TYPE_COLLECTOR))

#define CLIPMAN_COLLECTOR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CLIPMAN_TYPE_COLLECTOR, ClipmanCollectorClass))

typedef struct _ClipmanCollectorClass           ClipmanCollectorClass;
typedef struct _ClipmanCollector                ClipmanCollector;
typedef struct _ClipmanCollectorPrivate         ClipmanCollectorPrivate;

struct _ClipmanCollectorClass
{
  GObjectClass              parent_class;
};

struct _ClipmanCollector
{
  GObject                   parent;

  /* Private */
  ClipmanCollectorPrivate  *priv;
};

GType                   clipman_collector_get_type              ();

ClipmanCollector *      clipman_collector_get                   ();
void                    clipman_collector_set_is_restoring      (ClipmanCollector *collector);

#endif /* !__CLIPMAN_COLLECTOR_H__ */

