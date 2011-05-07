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

#ifndef __CLIPMAN_MENU_H__
#define __CLIPMAN_MENU_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define CLIPMAN_TYPE_MENU                  (clipman_menu_get_type())

#define CLIPMAN_MENU(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLIPMAN_TYPE_MENU, ClipmanMenu))
#define CLIPMAN_MENU_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CLIPMAN_TYPE_MENU, ClipmanMenuClass))

#define CLIPMAN_IS_MENU(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLIPMAN_TYPE_MENU))
#define CLIPMAN_IS_MENU_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CLIPMAN_TYPE_MENU))

#define CLIPMAN_MENU_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CLIPMAN_TYPE_MENU, ClipmanMenuClass))

typedef struct _ClipmanMenuClass           ClipmanMenuClass;
typedef struct _ClipmanMenu                ClipmanMenu;
typedef struct _ClipmanMenuPrivate         ClipmanMenuPrivate;

struct _ClipmanMenuClass
{
  GtkMenuClass           parent_class;
};

struct _ClipmanMenu
{
  GtkMenu                parent;

  /* Private */
  ClipmanMenuPrivate    *priv;
};

GType                   clipman_menu_get_type           ();

GtkWidget *             clipman_menu_new                ();

#endif /* !__CLIPMAN_MENU_H__ */

