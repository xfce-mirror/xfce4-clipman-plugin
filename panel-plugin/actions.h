/*
 *  Copyright (c) 2009 Mike Massonnet <mmassonnet@xfce.org>
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

#ifndef __CLIPMAN_ACTIONS_H__
#define __CLIPMAN_ACTIONS_H__

#include <glib-object.h>

/*
 * ClipmanActionsEntry struct declaration
 */

typedef struct _ClipmanActionsEntry ClipmanActionsEntry;
struct _ClipmanActionsEntry
{
  gchar                *action_name;
  GRegex               *regex;
  GHashTable           *commands;
};

/*
 * ClipmanActions GObject declaration
 */

#define CLIPMAN_TYPE_ACTIONS                  (clipman_actions_get_type())

#define CLIPMAN_ACTIONS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLIPMAN_TYPE_ACTIONS, ClipmanActions))
#define CLIPMAN_ACTIONS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CLIPMAN_TYPE_ACTIONS, ClipmanActionsClass))

#define CLIPMAN_IS_ACTIONS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLIPMAN_TYPE_ACTIONS))
#define CLIPMAN_IS_ACTIONS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CLIPMAN_TYPE_ACTIONS))

#define CLIPMAN_ACTIONS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CLIPMAN_TYPE_ACTIONS, ClipmanActionsClass))

typedef struct _ClipmanActionsClass           ClipmanActionsClass;
typedef struct _ClipmanActions                ClipmanActions;
typedef struct _ClipmanActionsPrivate         ClipmanActionsPrivate;

struct _ClipmanActionsClass
{
  GObjectClass              parent_class;
};

struct _ClipmanActions
{
  GObject                   parent;

  /* Private */
  ClipmanActionsPrivate    *priv;
};

GType                   clipman_actions_get_type               ();

ClipmanActions *      	clipman_actions_get                    ();
gboolean                clipman_actions_add                    (ClipmanActions *actions,
                                                                const gchar *action_name,
                                                                const gchar *regex,
                                                                const gchar *command_name,
                                                                const gchar *command);
gboolean                clipman_actions_remove                 (ClipmanActions *actions,
                                                                const gchar *action_name);
gboolean                clipman_actions_remove_command         (ClipmanActions *actions,
                                                                const gchar *action_name,
                                                                const gchar *command_name);
const GSList *          clipman_actions_get_entries            (ClipmanActions *actions);
GSList *                clipman_actions_match                  (ClipmanActions *actions,
                                                                const gchar *match);
void                    clipman_actions_match_with_menu        (ClipmanActions *actions,
                                                                const gchar *match);
void                    clipman_actions_load                   (ClipmanActions *actions);
void                    clipman_actions_save                   (ClipmanActions *actions);

#endif /* !__CLIPMAN_ACTIONS_H__ */

