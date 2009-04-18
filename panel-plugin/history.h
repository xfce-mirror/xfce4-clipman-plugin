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

#ifndef __CLIPMAN_HISTORY_H__
#define __CLIPMAN_HISTORY_H__

#include <glib-object.h>
#include <gtk/gtk.h>

/*
 * ClipmanHistoryItem
 */

typedef enum
{
  CLIPMAN_HISTORY_TYPE_TEXT,
  CLIPMAN_HISTORY_TYPE_IMAGE,
} ClipmanHistoryType;

typedef struct _ClipmanHistoryItem ClipmanHistoryItem;
struct _ClipmanHistoryItem
{
  ClipmanHistoryType        type;
  union
    {
      gchar                *text;
      GdkPixbuf            *image;
    } content;
  union
    {
      gchar                *text;
      GdkPixbuf            *image;
    } preview;
};

/*
 * ClipmanHistory
 */

#define CLIPMAN_TYPE_HISTORY                  (clipman_history_get_type())

#define CLIPMAN_HISTORY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLIPMAN_TYPE_HISTORY, ClipmanHistory))
#define CLIPMAN_HISTORY_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CLIPMAN_TYPE_HISTORY, ClipmanHistoryClass))

#define CLIPMAN_IS_HISTORY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLIPMAN_TYPE_HISTORY))
#define CLIPMAN_IS_HISTORY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CLIPMAN_TYPE_HISTORY))

#define CLIPMAN_HISTORY_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CLIPMAN_TYPE_HISTORY, ClipmanHistoryClass))

typedef struct _ClipmanHistoryClass           ClipmanHistoryClass;
typedef struct _ClipmanHistory                ClipmanHistory;
typedef struct _ClipmanHistoryPrivate         ClipmanHistoryPrivate;

struct _ClipmanHistoryClass
{
  GObjectClass              parent_class;

  void
  (*item_added)             (ClipmanHistory *history);
};

struct _ClipmanHistory
{
  GObject                   parent;

  /* Private */
  ClipmanHistoryPrivate    *priv;
};

GType                       clipman_history_get_type               ();

ClipmanHistory *            clipman_history_get                    ();
void                        clipman_history_add_text               (ClipmanHistory *history,
                                                                    const gchar *text);
void                        clipman_history_add_image              (ClipmanHistory *history,
                                                                    const GdkPixbuf *image);
GSList *                    clipman_history_get_list               (ClipmanHistory *history);
const ClipmanHistoryItem *  clipman_history_get_item_to_restore    (ClipmanHistory *history);
void                        clipman_history_set_item_to_restore    (ClipmanHistory *history,
                                                                    const ClipmanHistoryItem *item);
void                        clipman_history_clear                  (ClipmanHistory *history);

#endif /* !__CLIPMAN_HISTORY_H__ */

