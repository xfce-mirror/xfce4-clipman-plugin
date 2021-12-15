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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "common.h"
#include "history.h"
#include "secure_text.h"

#define  CLIPMAN_HISTORY_PREVIEW_LEN 48

/*
 * GObject declarations
 */

struct _ClipmanHistoryPrivate
{
  GList                        *items;
  const ClipmanHistoryItem     *item_to_restore;
  GList                        *last_item;
  guint                         nb_items;
  guint                         nb_texts;
  guint                         nb_images;
  guint                         max_texts_in_history;
  guint                         max_images_in_history;
  gboolean                      save_on_quit;
  gboolean                      reorder_items;
  // handling index in a circular buffer of pointer to GList item
  GList                       **indexes;
  GList                       **next_free_index;
  ClipmanHistoryId              indexes_nb_allocated_cells;
};

G_DEFINE_TYPE_WITH_PRIVATE (ClipmanHistory, clipman_history, G_TYPE_OBJECT)

enum
{
  MAX_TEXTS_IN_HISTORY = 1,
  MAX_IMAGES_IN_HISTORY,
  SAVE_ON_QUIT,
  REORDER_ITEMS,
};

enum
{
  ITEM_ADDED,
  CLEAR,
  LAST_SIGNAL,
};
static guint signals[LAST_SIGNAL];

static void             clipman_history_finalize           (GObject *object);
static void             clipman_history_set_property       (GObject *object,
                                                            guint property_id,
                                                            const GValue *value,
                                                            GParamSpec *pspec);
static void             clipman_history_get_property       (GObject *object,
                                                            guint property_id,
                                                            GValue *value,
                                                            GParamSpec *pspec);

/*
 * Private methods declarations
 */

static void            _clipman_history_add_item           (ClipmanHistory *history,
                                                            ClipmanHistoryItem *item);
static GList**         _clipman_history_indexes_find_next  (ClipmanHistory *history,
                                                            gboolean start_from_begining,
                                                            GList **skip_index);

/*
 * Misc functions declarations
 */

static void           __clipman_history_item_free          (ClipmanHistoryItem *item);
static gint           __g_list_compare_texts               (gconstpointer a,
                                                            gconstpointer b);

/*
 * Private methods
 */

static void
_clipman_history_indexes_reset_array (ClipmanHistory *history, gboolean set_null)
{
  history->priv->next_free_index = history->priv->indexes;
  history->priv->nb_items = 0;
  history->priv->nb_texts = 0;
  history->priv->nb_images = 0;

  if (set_null)
  {
    guint i;
    for(i = 0; i < history->priv->max_texts_in_history; i++)
      {
        history->priv->indexes[i] = NULL;
      }
  }
}

static void
_clipman_history_indexes_init_array (ClipmanHistory *history)
{
  // max_texts_in_history stand for the max size of the history
  history->priv->indexes_nb_allocated_cells = history->priv->max_texts_in_history;
  // init with NULL pointers
  history->priv->indexes = g_malloc0_n (history->priv->indexes_nb_allocated_cells,
                                        sizeof(GList**));
  _clipman_history_indexes_reset_array (history, FALSE);
}

static ClipmanHistoryResizeResult
_clipman_history_resize_history(ClipmanHistory *history, guint new_history_size)
{
  ClipmanHistoryResizeResult r;
  r.old_history_size = history->priv->max_texts_in_history;

  if (new_history_size == history->priv->max_texts_in_history)
    {
      r.resize_status = CLIPMAN_HISTORY_SIZE_UNCHANGED;
    }
  else
    {
      if (new_history_size > history->priv->max_texts_in_history)
        {
          // enlarge history
          // we only extend array, will be resized to max_texts_in_history
          // on clipman_history_clear() call only.
          if (new_history_size > history->priv->indexes_nb_allocated_cells)
          {
            guint i;
            guint next_free_index_id;

            if (history->priv->next_free_index != NULL)
              next_free_index_id = history->priv->next_free_index - history->priv->indexes;
            else
              next_free_index_id = history->priv->max_texts_in_history;

            history->priv->indexes_nb_allocated_cells = new_history_size;

            // copy indexes to a new larger array
            history->priv->indexes = g_realloc_n (history->priv->indexes,
                                                  new_history_size,
                                                  sizeof(GList**));
            // fatal error cannot realloac indexes
            if (history->priv->indexes == NULL)
              g_assert_not_reached ();

            // init to NULL enlarged history indexes
            for (i = history->priv->max_texts_in_history; i < new_history_size; i++)
              {
                history->priv->indexes[i] = NULL;
              }

            // move next_free_index point to the new reallocated memory
            history->priv->next_free_index = history->priv->indexes + next_free_index_id;
          }

          r.resize_status = CLIPMAN_HISTORY_SIZE_ENLARGE;
          if (history->priv->next_free_index == NULL)
          {
            // the history was full and we didn't reallocated memory but
            // indexes_nb_allocated_cells was wide enough to support the enlarge
            history->priv->next_free_index = history->priv->indexes +
                                             history->priv->max_texts_in_history;
          }
          history->priv->max_texts_in_history = new_history_size;

          // ensure that we are pointing to a free space, would happen if we are
          // pointing to an index that is keeping pointing to an item in the
          // list after shrink + enlarge without removal of item.
          history->priv->next_free_index =
              _clipman_history_indexes_find_next(history, FALSE, NULL);

          // fatal error wrong memory management no free index found
          if (history->priv->next_free_index == NULL)
            g_assert_not_reached ();
          if (*(history->priv->next_free_index) != NULL)
              g_assert_not_reached ();
        }
      else
        {
          // shrink history
          GList *link, *prev;
          guint nb_item_to_delete = 0;

          if (history->priv->nb_items > new_history_size)
          {
            nb_item_to_delete = history->priv->nb_items - new_history_size;
          }

          // indexes[] is not reallocated on shrink.
          // The history may have higher index values. Such ID wont be realloaced.
          // See: clipman_history_delete_item_by_pointer().
          history->priv->max_texts_in_history = new_history_size;

          // free all items from the end of the list
          for (link = history->priv->last_item; link != NULL && nb_item_to_delete > 0; link = prev)
            {
              // saving prev as the element will be deleted
              prev = link->prev;
              // history->priv->next_free_index is updated by the deletion
              // indexes are set to NULL during free.
              clipman_history_delete_item_by_pointer(history, link);
              nb_item_to_delete--;
            }

          // new reduced list is full so next_free_index should be NULL
          if (history->priv->nb_items == new_history_size)
          {
            if (history->priv->next_free_index != NULL)
              g_assert_not_reached();
          }


          r.resize_status = CLIPMAN_HISTORY_SIZE_SHRINK;
        }
    }

  r.max_texts_in_history = history->priv->max_texts_in_history;
  return r;
}


static void
_clipman_history_ensure_free_place_for_type(ClipmanHistory *history,
                                            ClipmanHistoryType type,
                                            guint nb_free_place_request)
{
  guint nb_free_place;
  ClipmanHistoryPrivate *priv = history->priv;

  // max_texts_in_history stands for max items in history
  nb_free_place =   priv->max_texts_in_history
                  - priv->nb_items;

  if (nb_free_place >= nb_free_place_request)
    {
      return;
    }
  else /* no free place, something needs to be removed */
    {
      gboolean remove_any_type = FALSE;
      GList *link, *prev;
      ClipmanHistoryItem *item;

      if (type == CLIPMAN_HISTORY_TYPE_IMAGE && priv->nb_images == 0)
        {
          // no image to remove but history full
          remove_any_type = TRUE;
        }

      // loop from the end of the list
      for (link = priv->last_item; link != NULL; link = prev)
        {
          // saving prev as the element could be deleted
          prev = link->prev;
          item = link->data;

          // CLIPMAN_HISTORY_TYPE_TEXT will also match CLIPMAN_HISTORY_TYPE_SECURE_TEXT
          // but CLIPMAN_HISTORY_TYPE_SECURE_TEXT will only match SECURE_TEXT
          if (   remove_any_type
              || item->type == type
              || (  type == CLIPMAN_HISTORY_TYPE_TEXT
                 && item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT )
             )
          {
            DBG ("Delete oldest content from the history (%d) of type: %s, requested: %s",
                 item->id,
                 clipman_history_is_text_item(item) ? "TEXT" : "IMAGE",
                 type == CLIPMAN_HISTORY_TYPE_TEXT ? "TEXT" : "IMAGE"
                );
            clipman_history_delete_item_by_pointer (history, link);
            nb_free_place++;
          }

          if (nb_free_place >= nb_free_place_request)
            break;
        }

      // DONE by clipman_history_delete_item_by_pointer ?
      // priv->last_item = prev;
    }
}

static void
_clipman_history_add_item (ClipmanHistory *history,
                           ClipmanHistoryItem *item)
{
  /* Prepend item to start of the history */
  history->priv->items = g_list_prepend (history->priv->items, item);
  // chain the indexes pointer to the new item in the GList
  history->priv->indexes[item->id-1] = history->priv->items;
  history->priv->item_to_restore = item;
  if (history->priv->last_item == NULL)
    {
      // initialize last_item, the list was empty
      history->priv->last_item = history->priv->items;
    }

  history->priv->nb_items++;
  if (clipman_history_is_text_item(item))
      history->priv->nb_texts++;
  else if (item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
      history->priv->nb_images++;

  /* Emit signal */
  g_signal_emit (history, signals[ITEM_ADDED], 0);
}

/*
 * Misc functions
 */

gboolean
clipman_history_is_text_item(ClipmanHistoryItem *item)
{
  return     item->type == CLIPMAN_HISTORY_TYPE_TEXT
         ||  item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT;
}

static void
__clipman_history_item_free (ClipmanHistoryItem *item)
{
  switch (item->type)
    {
    case CLIPMAN_HISTORY_TYPE_TEXT:
    case CLIPMAN_HISTORY_TYPE_SECURE_TEXT:
      DBG ("Delete text `%s'", item->content.text);
      g_free (item->content.text);
      g_free (item->preview.text);
      break;

    case CLIPMAN_HISTORY_TYPE_IMAGE:
      DBG ("Delete image (%p)", item->content.image);
      g_object_unref (item->content.image);
      g_object_unref (item->preview.image);
      break;

    default:
      g_assert_not_reached ();
    }
  g_slice_free (ClipmanHistoryItem, item);
}

static gint
__g_list_compare_texts (gconstpointer a,
                         gconstpointer b)
{
  const ClipmanHistoryItem *item = a;
  const gchar *text = b;
  switch (item->type)
    {
    case CLIPMAN_HISTORY_TYPE_IMAGE:
      return -1;
      break;

    case CLIPMAN_HISTORY_TYPE_TEXT:
    case CLIPMAN_HISTORY_TYPE_SECURE_TEXT:
      return g_ascii_strcasecmp (item->content.text, text);
      break;

    default:
      g_assert_not_reached ();
    }
}

/**
 * = Private method =
 * _clipman_history_indexes_find_next:
 * @history:             a #ClipmanHistory
 * @start_from_begining: a #gboolean
 * @skip_index:          a #GList pointer pointer to skip
 *
 * Returns: a #GList pointer pointer
 *
 * Loop into history->priv->indexes circular buffer starting at current
 * next_free_index for a free cell (content pointing to NULL). Return the pointer
 * to the first found or NULL if no place is left. history->priv->next_free_index
 * is left unchanged.
 */
static GList**
_clipman_history_indexes_find_next(ClipmanHistory *history,
                                   gboolean start_from_begining,
                                   GList **skip_index)
{
  GList **next_free_index, **last_index;

  // no place left in the indexes
  if (   ! start_from_begining
      && ( history->priv->max_texts_in_history - history->priv->nb_items == 0
      ||   history->priv->next_free_index == NULL )
     )
    {
      return NULL;
    }

  last_index = history->priv->indexes + (history->priv->max_texts_in_history - 1);
  if (start_from_begining)
  {
    next_free_index = history->priv->indexes;
  }
  else
  {
    next_free_index = history->priv->next_free_index;
    if (next_free_index == skip_index)
      {
        if (next_free_index == last_index)
            next_free_index = history->priv->indexes;
        else
            next_free_index ++;
      }
  }

  // there should be some free cell so we shouldn't loop for ever.
  while (*next_free_index != NULL)
    {
      if (next_free_index == last_index)
        {
          if (skip_index == NULL)
            {
              // we finished the scan from the beginning
              break;
            }
          else
            {
              // loop start at the beginning of our circular buffer
              next_free_index = history->priv->indexes;
            }
        }
      else
        {
          next_free_index++;
        }
    }

  // next_free_index shouldn't never be pointing to a non free cell
  // but in case of skip_index == NULL the loop exit condition on last_item
  // could be reached. This case is an error.
  return *next_free_index == NULL ? next_free_index : NULL;
}

/**
 * = Private method =
 * _clipman_history_get_next_id
 * @history:    a #ClipmanHistory
 *
 * Returns: a #ClipmanHistoryId    value 0 means error
 *
 * Check into the history->priv->indexes for a free ID. The computed value
 * of the ID is returned between 1 and max_texts_in_history included.
 * Or 0 if no free ID is available, which is an error.
 *
 * The internal pointer to next_free_index is moved forward or set to NULL if no
 * more free indexes is reached.
 */
static ClipmanHistoryId
_clipman_history_get_next_id(ClipmanHistory *history)
{
  ClipmanHistoryId next_id;
  int nb_free_index;
  GList **next_free_index;

  next_free_index = history->priv->next_free_index;
  nb_free_index = history->priv->max_texts_in_history - history->priv->nb_items;
  if (nb_free_index <= 0 || next_free_index == NULL)
    {
      // error
      DBG("_clipman_history_get_next_id() error: no free index");
      return 0;
    }

  next_id = (next_free_index - history->priv->indexes) + 1;
  nb_free_index--;

  // move to the next free indexes in the buffer.
  if (nb_free_index > 0)
    {
      history->priv->next_free_index = _clipman_history_indexes_find_next(
                                              history,
                                              FALSE,
                                              next_free_index
                                              );
    }
  else
    {
      history->priv->next_free_index = NULL;
    }

  return next_id;
}

static void
_clipman_history_set_preview_text(ClipmanHistoryItem *item)
{
  gchar *tmp1;

  if (! clipman_history_is_text_item(item))
    return;

  if(item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT)
  {
    // vim: i_CTRL-v + u + 4 digit or i_CTRL-v + U + 8 digit 😀
    // utf-8 symbol:
    //  Wrong way sign:  0x26d4 ⛔
    //  Locker with key: 0x0001f510 🔐
    // require font with emoji: sudo apt install fonts-emojione
    tmp1 = g_strdup_printf ("🔐 SECURE %2d *********", item->id);
  }
  else
  {
    const gchar *offset;
    gchar *tmp2;

    /* Strip white spaces for preview */
    tmp1 = g_strchomp (g_strdup (item->content.text));

    tmp2 = tmp1;
    while (tmp2)
      {
        tmp2 = g_strchug(++tmp2);
        tmp2 = g_strstr_len (tmp1, CLIPMAN_HISTORY_PREVIEW_LEN, "  ");
      }

    /* Shorten preview */
    if (g_utf8_strlen (tmp1, -1) > CLIPMAN_HISTORY_PREVIEW_LEN)
      {
        offset = g_utf8_offset_to_pointer (tmp1, CLIPMAN_HISTORY_PREVIEW_LEN);
        tmp2 = g_strndup (tmp1, offset - tmp1);
        g_free (tmp1);
        tmp1 = g_strconcat (tmp2, "...", NULL);
        g_free (tmp2);
      }

    /* Cleanup special characters from preview */
    tmp1 = g_strdelimit (tmp1, "\n\r\t", ' ');
  }

  if (item->preview.text != NULL)
    g_free (item->preview.text);
  item->preview.text = tmp1;
}

/*
 * Public methods
 */

/**
 * clipman_history_add_text:
 * @history:    a #ClipmanHistory
 * @text:       the text to add to the history (encoded, if is_secure is TRUE)
 * @is_secure:  a #Boolean to indicate if the new text item is a SECURE_TEXT
 *
 * Stores a text inside the history.  If the history is growing over the
 * maximum number of items, it will delete the oldest text.
 */
ClipmanHistoryId
clipman_history_add_text (ClipmanHistory *history,
                          gboolean is_secure,
                          const gchar *text)
{
  ClipmanHistoryItem *item;
  GList *link;

  /* Search for a previously existing content */
  link = g_list_find_custom (history->priv->items, text, (GCompareFunc)__g_list_compare_texts);
  if (link != NULL)
    {
      item = link->data;
      DBG ("Found a previous occurence for text: `%s' id: %d is_secure: %s",
          text,
          item-> id,
          (item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT) ? "YES" : "NO");

      if (history->priv->reorder_items)
        {
          // move the item in the first position of the GList
          history->priv->items = g_list_remove_link (history->priv->items, link);
          history->priv->items = g_list_concat (link, history->priv->items);
        }

      // we force is_secure, if SECURE_TEXT is present on the new or the old
      // item.
      is_secure = (item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT) || is_secure;
      if (clipman_history_change_secure_text_state(history, is_secure, item))
        {
          DBG ("item id: %d became SECURE_TEXT", item->id);
        }

      history->priv->item_to_restore = item;
      return item->id;
    }

  /* Store the text */
  DBG ("Store text `%s')", text);

  item = g_slice_new0 (ClipmanHistoryItem);
  item->content.text = g_strdup(text);
  // setting preview at NULL ensure that it will be set by clipman_history_change_secure_text_state()
  // g_slice_new0 should set it, but it's good to have it explicitly set for human memory
  item->preview.text = NULL;
  if (is_secure)
    {
      // text should be already encoded if is_secure == TRUE
      item->type = CLIPMAN_HISTORY_TYPE_SECURE_TEXT;
    }
  else
    {
      item->type = CLIPMAN_HISTORY_TYPE_TEXT;
    }

  // _clipman_history_get_next_id must have a free place to return an ID
  _clipman_history_ensure_free_place_for_type(history, CLIPMAN_HISTORY_TYPE_TEXT, 1);
  item->id = _clipman_history_get_next_id(history);
  if (item->id == 0)
    g_assert_not_reached();
  // setting preview to a SECURE_TEXT uses the ID
  _clipman_history_set_preview_text(item);

  // this last method will emit the changed signal for GUI
  _clipman_history_add_item (history, item);

  return item->id;
}

/**
 * clipman_history_add_image:
 * @history:    a #ClipmanHistory
 * @image:      the image to add to the history
 *
 * Stores an image inside the history.  If the history is growing over the
 * maximum number of items, it will delete the oldest image, if no image, then
 * the any oldest item. It ensures that there's at most max_images_in_history.
 */
void
clipman_history_add_image (ClipmanHistory *history,
                           const GdkPixbuf *image)
{
  ClipmanHistoryItem *item;

  if (history->priv->max_images_in_history == 0)
    return;

  DBG ("Store image (%p)", image);

  item = g_slice_new0 (ClipmanHistoryItem);
  item->type = CLIPMAN_HISTORY_TYPE_IMAGE;
  item->content.image = gdk_pixbuf_copy (image);
  item->preview.image = gdk_pixbuf_scale_simple (GDK_PIXBUF (image), 128, 128, GDK_INTERP_BILINEAR);

  DBG ("Copy of image (%p) is (%p)", image, item->content.image);

  if (history->priv->nb_images >= history->priv->max_images_in_history)
    {
      _clipman_history_ensure_free_place_for_type(history, CLIPMAN_HISTORY_TYPE_IMAGE, 1);
    }
  else
    {
      // we can put more image in the history so we remove text item, if needed
      _clipman_history_ensure_free_place_for_type(history, CLIPMAN_HISTORY_TYPE_TEXT, 1);
    }

  item->id = _clipman_history_get_next_id(history);
  _clipman_history_add_item (history, item);
}

/**
 * clipman_history_get_list:
 * @history: a #ClipmanHistory
 *
 * Returns a unique list of the images and texts.
 *
 * Returns: a newly allocated #GList that must be freed with g_list_free()
 */
GList *
clipman_history_get_list (ClipmanHistory *history)
{
  return g_list_copy (history->priv->items);
}

/**
 * clipman_history_get_max_texts_in_history:
 * @history: a #ClipmanHistory
 *
 * Returns the most recent item that has been added to #ClipmanHistory.
 *
 * Returns: a #const #ClipmanHistoryItem
 */
guint
clipman_history_get_max_texts_in_history (ClipmanHistory *history)
{
  return history->priv->max_texts_in_history;
}

/**
 * clipman_history_get_item_to_restore:
 * @history: a #ClipmanHistory
 *
 * Returns the most recent item that has been added to #ClipmanHistory.
 *
 * Returns: a #const #ClipmanHistoryItem
 */
const ClipmanHistoryItem *
clipman_history_get_item_to_restore (ClipmanHistory *history)
{
  return history->priv->item_to_restore;
}

/**
 * clipman_history_set_item_to_restore:
 * @history: a #ClipmanHistory
 * @item: a #ClipmanHistoryItem that must exist inside #ClipmanHistory
 *
 * This function is used as a hack around the images as they cannot be
 * compared.  Before setting the clipboard with an image, the #ClipmanCollector
 * must be set as being restored with clipman_collector_set_is_restoring(),
 * than this function is called with the item that contains the image that is
 * getting restored.
 * Instead of being destroyed/recreated inside the history, it will remain at
 * the same position in the history (unlike being pushed to the top) but will
 * be marked as being the most recent item in the history.
 */
void
clipman_history_set_item_to_restore (ClipmanHistory *history,
                                     const ClipmanHistoryItem *item)
{
  /* TODO Verify that the item exists in the history */
  history->priv->item_to_restore = item;
}

/**
 * clipman_history_clear:
 * @history: a #ClipmanHistory
 * @clear_only_secure_text: a #Boolean to limit clearing to only SECURE_TEXT item
 *
 * Clears the lists containing the history of the texts and the images.
 */
guint
clipman_history_clear (ClipmanHistory *history, gboolean clear_only_secure_text)
{
  GList *list, *next;
  guint nb_element_cleared = 0;

  if (clear_only_secure_text)
    {
      ClipmanHistoryItem *item;
      DBG ("Clear the history: only_secure_text");

      for (list = history->priv->items; list != NULL; list = next)
      {
        item = list->data;
        next = list->next;
        if (item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT)
          {
            // clipman_history_delete_item_by_pointer() also free the item
            // NOTE, this also changes the list we are looping.
            clipman_history_delete_item_by_pointer (history, list);
            nb_element_cleared ++;
          }
      }
      // signals are emited at each delete call
    }
  else
    {
      DBG ("Clear the history: all items");

      for (list = history->priv->items; list != NULL; list = list->next)
        {
          __clipman_history_item_free (list->data);
          nb_element_cleared ++;
        }

      g_list_free (history->priv->items);
      history->priv->items = NULL;
      history->priv->item_to_restore = NULL;
      history->priv->last_item = NULL;
      _clipman_history_indexes_reset_array (history, TRUE);

      g_signal_emit (history, signals[CLEAR], 0);
    }

  return nb_element_cleared;
}

ClipmanHistory *
clipman_history_get (void)
{
  static ClipmanHistory *singleton = NULL;

  if (singleton == NULL)
    {
      singleton = g_object_new (CLIPMAN_TYPE_HISTORY, NULL);
      g_object_add_weak_pointer (G_OBJECT (singleton), (gpointer)&singleton);
    }
  else
    g_object_ref (G_OBJECT (singleton));

  return singleton;
}

/**
 * clipman_history_find_item_by_preview:
 * @history: a #ClipmanHistory
 * @preview_searched: a @gchar* the text of the preview we are looking for
 *
 * Returns: a pointer to the real GList link element in the list (not a copy, so it can be deleted)
 */
GList *
clipman_history_find_item_by_preview(ClipmanHistory *history, const gchar* preview_searched)
{
  GList *current;
  ClipmanHistoryItem *_item;

  /* search the item by its id in the list */
  for (current = history->priv->items; current != NULL; current = current->next)
    {
      _item = current->data;
      if (clipman_history_is_text_item(_item) &&
          g_strcmp0(_item->preview.text, preview_searched) == 0)
        {
          return current;
        }
    }

  // not found
  return NULL;
}
/**
 * clipman_history_find_item_by_id:
 * @history: a #ClipmanHistory
 * @searched_id: a #ClipmanHistoryId in the indexes
 *
 * Returns: a pointer to the real GList link element in the list (not a copy, so it can be deleted)
 */
GList *
clipman_history_find_item_by_id(ClipmanHistory *history, ClipmanHistoryId searched_id)
{
  // searched_id must be in the range of our circular buffer
  if (searched_id < 1 || searched_id > history->priv->indexes_nb_allocated_cells)
    return NULL;

  // returns the element at position searched_id that can be NULL too.
  return history->priv->indexes[searched_id-1];
}

gboolean
clipman_history_delete_item_by_id(ClipmanHistory *history, ClipmanHistoryId id)
{
  GList *link = clipman_history_find_item_by_id(history, id);
  if (link != NULL)
    {
      clipman_history_delete_item_by_pointer (history, link);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

/**
 * clipman_history_delete_item_by_pointer:
 * This is the mail deleting item entry point. Everything must be maintained by
 * this function when deleting an item in the list:
 *   items the GList head
 *   counters: nb_items, nb_texts, nb_images, etc.
 *   last_item the GList queue
 *   item_to_restore
 *   indexes[] pointers
 *   next_free_index
 *
 * @history a #ClipmanHistory
 * @link    the @GList pointer that going to be removed (must exists)
 */
void
clipman_history_delete_item_by_pointer(ClipmanHistory *history, GList *link)
{
  ClipmanHistoryItem *item;
  GList **free_index;
  ClipmanHistoryPrivate *priv;

  priv = history->priv;
  item = link->data;

  // make the index at position item->id available in the circular buffer
  free_index = priv->indexes + (item->id-1);
  *free_index = NULL;
  if (priv->next_free_index == NULL)
    {
      // we freeed the last element, so it becomes the new next_free_index
      // or we look for lower indexes at the beginning of the circular buffer.

      if (item->id > priv->max_texts_in_history)
      {
        // Dont reallocate such higher ID, they have been kept by convinience
        // but are now larger than the history size.
        if (priv->nb_items <= priv->max_texts_in_history)
          {
            // If the history is not full there should have some free lower ID
            // left at the begining of the history.
            priv->next_free_index = _clipman_history_indexes_find_next(
                                                    history,
                                                    TRUE,
                                                    NULL);
            // fatal error
            if (priv->next_free_index == NULL)
              g_assert_not_reached ();
          }
        // else
        // when called from _clipman_history_resize_history() during the deletion
        // next_free_index may stay NULL as no free place is available
      }
      else
      {
        // when called from _clipman_history_resize_history() during the deletion
        // next_free_index may stay NULL until some free place becomes  available
        if (priv->nb_items <= priv->max_texts_in_history)
          priv->next_free_index = free_index;
      }
    }

  if (priv->nb_items > 1 && priv->item_to_restore == item)
    {
      if (link != priv->items)
        priv->item_to_restore = priv->items->data;
      else
        // as we have nb_items > 1, there should be a next
        priv->item_to_restore = priv->items->next->data;
    }
  else
    {
      // GList will become empty
      priv->item_to_restore = NULL;
    }

  if (clipman_history_is_text_item (item))
    {
      priv->nb_texts--;
    }
  else if (item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
    {
      priv->nb_images--;
    }

  // free memory used by the item pointed in the list
  __clipman_history_item_free (item);

  // update our last_item pointer
  if (link == priv->last_item)
    {
      priv->last_item = link->prev;
    }

  // this will eventually update the list first item pointer
  priv->items = g_list_delete_link (priv->items, link);
  priv->nb_items--;

  /* Emit signal for redraw menu */
  g_signal_emit (history, signals[ITEM_ADDED], 0);
}

gboolean
clipman_history_change_secure_text_state(ClipmanHistory * history,
                                         gboolean secure,
                                         ClipmanHistoryItem *item)
{
  gchar *old_text;
  gboolean changed;

  changed = FALSE;
  old_text = item->content.text;

  if (secure && item->type == CLIPMAN_HISTORY_TYPE_TEXT)
  {
    item->content.text = clipman_secure_text_encode(old_text);
    item->type = CLIPMAN_HISTORY_TYPE_SECURE_TEXT;
    changed = TRUE;
  }
  else if (!secure && item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT)
  {
    item->content.text = clipman_secure_text_decode(old_text);
    item->type = CLIPMAN_HISTORY_TYPE_TEXT;
    changed = TRUE;
  }

  if (changed)
  {
    g_free(old_text);
  }

  if (changed || item->preview.text == NULL)
  {
    _clipman_history_set_preview_text(item);
    g_signal_emit (history, signals[ITEM_ADDED], 0);
  }

  return changed;
}

ClipmanHistoryResizeResult
clipman_history_set_history_size(ClipmanHistory *history, guint size)
{
  ClipmanHistoryResizeResult r;
  if (history->priv->indexes == NULL)
    {
      // first time creating the index
      history->priv->max_texts_in_history = size;
      _clipman_history_indexes_init_array(history);
      r.resize_status = CLIPMAN_HISTORY_SIZE_CREATE;
      r.old_history_size = 0;
      r.max_texts_in_history = size;
    }
  else
    {
      // history size changed, we adapt to the new size
      r = _clipman_history_resize_history(history, size);
    }
  return r;
}

/*
 * GObject
 */

static void
clipman_history_class_init (ClipmanHistoryClass *klass)
{
  GObjectClass *object_class;

  clipman_history_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = clipman_history_finalize;
  object_class->set_property = clipman_history_set_property;
  object_class->get_property = clipman_history_get_property;

  signals[ITEM_ADDED] =
    g_signal_new ("item-added", G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ClipmanHistoryClass, item_added),
                  0, NULL, g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[CLEAR] =
    g_signal_new ("clear", G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ClipmanHistoryClass, clear),
                  0, NULL, g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class,
                                   MAX_TEXTS_IN_HISTORY,
                                   g_param_spec_uint ("max-texts-in-history",
                                                      "MaxTextsInHistory",
                                                      "The number of maximum texts in history",
                                                      5, 1000, DEFAULT_MAX_TEXTS_IN_HISTORY,
                                                      G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   MAX_IMAGES_IN_HISTORY,
                                   g_param_spec_uint ("max-images-in-history",
                                                      "MaxImagesInHistory",
                                                      "The number of maximum images in history",
                                                      0, 5, DEFAULT_MAX_IMAGES_IN_HISTORY,
                                                      G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   SAVE_ON_QUIT,
                                   g_param_spec_boolean ("save-on-quit",
                                                         "SaveOnQuit",
                                                         "True if the history must be saved on quit",
                                                         DEFAULT_SAVE_ON_QUIT,
                                                         G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   REORDER_ITEMS,
                                   g_param_spec_boolean ("reorder-items",
                                                         "ReorderItems",
                                                         "Always push last clipboard content to the top of the history",
                                                         DEFAULT_REORDER_ITEMS,
                                                         G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
}

// clipman_history_init() is called automatically by clipman_history_class_init
// but the value are not set yet
static void
clipman_history_init (ClipmanHistory *history)
{
  history->priv = clipman_history_get_instance_private (history);
  history->priv->item_to_restore = NULL;
  history->priv->indexes = NULL;
}

static void
clipman_history_finalize (GObject *object)
{
  clipman_history_clear (CLIPMAN_HISTORY (object), FALSE);
  G_OBJECT_CLASS (clipman_history_parent_class)->finalize (object);
}

static void
clipman_history_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  ClipmanHistoryPrivate *priv = CLIPMAN_HISTORY (object)->priv;

  switch (property_id)
    {
    case MAX_TEXTS_IN_HISTORY:
      {
        guint new_history_size = g_value_get_uint (value);
        clipman_history_set_history_size(CLIPMAN_HISTORY (object), new_history_size);
      }
      break;

    case MAX_IMAGES_IN_HISTORY:
      priv->max_images_in_history = g_value_get_uint (value);
      break;

    case SAVE_ON_QUIT:
      priv->save_on_quit = g_value_get_boolean (value);
      break;

    case REORDER_ITEMS:
      priv->reorder_items = g_value_get_boolean (value);
      break;

    default:
      break;
    }
}

static void
clipman_history_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  ClipmanHistoryPrivate *priv = CLIPMAN_HISTORY (object)->priv;

  switch (property_id)
    {
    case MAX_TEXTS_IN_HISTORY:
      g_value_set_uint (value, priv->max_texts_in_history);
      break;

    case MAX_IMAGES_IN_HISTORY:
      g_value_set_uint (value, priv->max_images_in_history);
      break;

    case SAVE_ON_QUIT:
      g_value_set_boolean (value, priv->save_on_quit);
      break;

    case REORDER_ITEMS:
      g_value_set_boolean (value, priv->reorder_items);
      break;

    default:
      break;
    }
}
