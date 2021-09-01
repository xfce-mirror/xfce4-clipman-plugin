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
  guint                         max_texts_in_history;
  guint                         max_images_in_history;
  gboolean                      save_on_quit;
  gboolean                      reorder_items;
  ClipmanHistoryId              current_id;
  ClipmanHistoryId              max_id_value;
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
_clipman_history_add_item (ClipmanHistory *history,
                           ClipmanHistoryItem *item)
{
  GList *list;
  ClipmanHistoryItem *_item;
  guint list_length;
  guint n_texts = 0;
  guint n_images = 0;

  /* Count initial items */
  for (list = history->priv->items; list != NULL; list = list->next)
    {
      _item = list->data;
      if (_item->type == CLIPMAN_HISTORY_TYPE_TEXT || _item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT)
        {
          n_texts++;
        }
      else if (_item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
        {
          n_images++;
        }
    }

  list_length = n_texts + n_images;

  /* First truncate history to max_items (max_texts stands for the size of the history) */
  while (list_length > history->priv->max_texts_in_history)
    {
      DBG ("Delete oldest content from the history");
      list = g_list_last (history->priv->items);
      _item = list->data;

      if (_item->type == CLIPMAN_HISTORY_TYPE_TEXT || _item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT)
        {
          n_texts--;
        }
      else if (_item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
        {
          n_images--;
        }
      list_length--;

      __clipman_history_item_free (_item);
      history->priv->items = g_list_remove (history->priv->items, _item);
    }

  /* Free last image from history if max_images is reached, otherwise last item from history */
  if (item->type == CLIPMAN_HISTORY_TYPE_IMAGE && n_images >= history->priv->max_images_in_history)
    {
      while (n_images >= history->priv->max_images_in_history)
        {
          guint i = 1;

          for (list = history->priv->items; list != NULL; list = list->next)
            {
              _item = list->data;

              if (_item->type != CLIPMAN_HISTORY_TYPE_IMAGE)
                continue;

              i++;

              if (i < n_images)
                continue;

              if (n_images >= history->priv->max_images_in_history)
                {
                  __clipman_history_item_free (_item);
                  history->priv->items = g_list_remove (history->priv->items, _item);
                }
              n_images--;

              break;
            }
        }
    }
  else if (list_length == history->priv->max_texts_in_history)
    {
      list = g_list_last (history->priv->items);
      _item = list->data;
      __clipman_history_item_free (_item);
      history->priv->items = g_list_remove (history->priv->items, _item);
    }

  /* Prepend item to start of the history */
  history->priv->items = g_list_prepend (history->priv->items, item);
  history->priv->item_to_restore = item;

  /* Emit signal */
  g_signal_emit (history, signals[ITEM_ADDED], 0);
}

/*
 * Misc functions
 */

static gboolean
_clipman_history_is_text_item(ClipmanHistoryItem *item)
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

static ClipmanHistoryId
_clipman_history_get_next_id(ClipmanHistory *history)
{
  ClipmanHistoryId next_id;
  if (history->priv->current_id < history->priv->max_id_value)
    {
      next_id = history->priv->current_id + 1;
    }
  else
    {
      // loop id counter
      next_id = 1;
    }
  history->priv->current_id = next_id;
  return next_id;
}

static void
_clipman_history_set_preview_text(ClipmanHistoryItem *item)
{
  gchar *tmp1;

  if (! _clipman_history_is_text_item(item))
    return;

  if(item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT)
  {
    // vim: i_CTRL-v + u + 4 digit or i_CTRL-v + U + 8 digit 😀
    // utf-8 symbol:
    //  Wrong way sign:  0x26d4 ⛔
    //  Locker with key: 0x0001f510 🔐
    // require font with emoji: sudo apt install fonts-emojione
    tmp1 = g_strdup_printf ("🔐 SECURE ***********");
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
 * @text:       the text to add to the history
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
  GList *list;

  /* Search for a previously existing content */
  list = g_list_find_custom (history->priv->items, text, (GCompareFunc)__g_list_compare_texts);
  if (list != NULL)
    {
      item = list->data;
      DBG ("Found a previous occurence for text `%s' is_secure: %s", text,
          (item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT) ? "YES" : "NO");
      // we force is_secure: if the found item was secure or if the new item is secure
      is_secure = (item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT) || is_secure;
      if (history->priv->reorder_items)
        {
          __clipman_history_item_free (item);
          history->priv->items = g_list_remove (history->priv->items, list->data);
        }
      else
        {
          history->priv->item_to_restore = item;
          return item->id;
        }
    }

  /* Store the text */
  DBG ("Store text `%s')", text);

  item = g_slice_new0 (ClipmanHistoryItem);

  item->content.text = g_strdup(text);
  // setting preview at NULL ensure that it will be set by clipman_history_change_secure_text_state()
  // g_slice_new0 should set it, but it's good to have it explicitly set for human memory
  item->preview.text = NULL;
  item->type = CLIPMAN_HISTORY_TYPE_TEXT;
  // will also set the preview accordingly
  clipman_history_change_secure_text_state(history, is_secure, item);
  item->id = _clipman_history_get_next_id(history);

  _clipman_history_add_item (history, item);

  return item->id;
}

/**
 * clipman_history_add_image:
 * @history:    a #ClipmanHistory
 * @image:      the image to add to the history
 *
 * Stores an image inside the history.  If the history is growing over the
 * maximum number of items, it will delete the oldest image.
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

// Sylvain added public method

/*
 * Returns: a pointer to the real GList link element in the list (not a copy, so it can be deleted)
 */
GList *
clipman_history_find_item_by_id(ClipmanHistory *history, ClipmanHistoryId searched_id)
{
  GList *current;
  ClipmanHistoryItem *_item;

  /* search the item by its id in the list */
  for (current = history->priv->items; current != NULL; current = current->next)
    {
      _item = current->data;
      if (_item->id == searched_id)
        {
          return current;
        }
    }

  // not found
  return NULL;
}

gboolean
clipman_history_delete_item_by_id(ClipmanHistory *history, ClipmanHistoryId id)
{
  GList *_link = clipman_history_find_item_by_id(history, id);
  if (_link != NULL)
    {
      clipman_history_delete_item_by_pointer (history, _link);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

void
clipman_history_delete_item_by_pointer(ClipmanHistory *history, GList *_link)
{
  // free memory used by the element pointed in the list
  __clipman_history_item_free ((ClipmanHistoryItem *)_link->data);
  // this will eventually update the list first item pointer
  history->priv->items = g_list_delete_link (history->priv->items, _link);

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
  // TODO: g_object_class_install_property with max_id_value ==> Sed clipman_history_init()
}

static void
clipman_history_init (ClipmanHistory *history)
{
  history->priv = clipman_history_get_instance_private (history);
  history->priv->item_to_restore = NULL;
  // start at 0, will get 1 on first call of _clipman_history_get_next_id()
  history->priv->current_id = 0;
  // TODO: forced value here, could be an property see g_object_class_install_property()
  history->priv->max_id_value = 55;
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
      priv->max_texts_in_history = g_value_get_uint (value);
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
