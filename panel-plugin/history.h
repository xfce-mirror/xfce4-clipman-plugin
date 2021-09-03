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

#ifndef __CLIPMAN_HISTORY_H__
#define __CLIPMAN_HISTORY_H__

#include <glib-object.h>
#include <gtk/gtk.h>

/*
 * ClipmanHistoryItem
 */

typedef enum
{
  CLIPMAN_HISTORY_TYPE_TEXT = 1,
  CLIPMAN_HISTORY_TYPE_IMAGE,
  CLIPMAN_HISTORY_TYPE_SECURE_TEXT,
} ClipmanHistoryType;

typedef gushort ClipmanHistoryId;
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
  ClipmanHistoryId          id;
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
  void
  (*clear)                  (ClipmanHistory *history);
};

struct _ClipmanHistory
{
  GObject                   parent;

  /* Private */
  ClipmanHistoryPrivate    *priv;
};

GType                       clipman_history_get_type                 (void);

ClipmanHistory *            clipman_history_get                      (void);
ClipmanHistoryId            clipman_history_add_text                 (ClipmanHistory *history,
                                                                      gboolean is_secure,
                                                                      const gchar *text);
void                        clipman_history_add_image                (ClipmanHistory           *history,
                                                                      const GdkPixbuf          *image);
GList *                     clipman_history_get_list                 (ClipmanHistory           *history);
guint                       clipman_history_get_max_texts_in_history (ClipmanHistory           *history);
const ClipmanHistoryItem *  clipman_history_get_item_to_restore      (ClipmanHistory           *history);
void                        clipman_history_set_item_to_restore      (ClipmanHistory           *history,
                                                                      const ClipmanHistoryItem *item);
guint                       clipman_history_clear                    (ClipmanHistory *history,
                                                                      gboolean clear_only_secure_text);
GList *                     clipman_history_find_item_by_id          (ClipmanHistory *history,
                                                                      ClipmanHistoryId searched_id);
gboolean                    clipman_history_delete_item_by_id        (ClipmanHistory *history, ClipmanHistoryId id);
void                        clipman_history_delete_item_by_pointer   (ClipmanHistory *history, GList *_link);
gboolean                    clipman_history_change_secure_text_state (ClipmanHistory * history,
                                                                      gboolean secure, ClipmanHistoryItem *item);
gboolean                    clipman_history_is_text_item             (ClipmanHistoryItem *item);

#endif /* !__CLIPMAN_HISTORY_H__ */
