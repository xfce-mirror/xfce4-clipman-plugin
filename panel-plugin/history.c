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

/*
 * GObject declarations
 */

struct _ClipmanHistoryPrivate
{
  GSList                       *items;
  const ClipmanHistoryItem     *item_to_restore;
  guint                         max_texts_in_history;
  guint                         max_images_in_history;
  gboolean                      save_on_quit;
  gboolean                      reorder_items;
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
static gint           __g_slist_compare_texts              (gconstpointer a,
                                                            gconstpointer b);



/*
 * Private methods
 */

static void
_clipman_history_add_item (ClipmanHistory *history,
                           ClipmanHistoryItem *item)
{
  GSList *list;
  ClipmanHistoryItem *_item;
  guint list_length;
  guint n_texts = 0;
  guint n_images = 0;

  /* Count initial items */
  for (list = history->priv->items; list != NULL; list = list->next)
    {
      _item = list->data;
      if (_item->type == CLIPMAN_HISTORY_TYPE_TEXT)
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
      list = g_slist_last (history->priv->items);
      _item = list->data;

      if (_item->type == CLIPMAN_HISTORY_TYPE_TEXT)
        {
          n_texts--;
        }
      else if (_item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
        {
          n_images--;
        }
      list_length--;

      __clipman_history_item_free (_item);
      history->priv->items = g_slist_remove (history->priv->items, _item);
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
                  history->priv->items = g_slist_remove (history->priv->items, _item);
                }
              n_images--;

              break;
            }
        }
    }
  else if (list_length == history->priv->max_texts_in_history)
    {
      list = g_slist_last (history->priv->items);
      _item = list->data;
      __clipman_history_item_free (_item);
      history->priv->items = g_slist_remove (history->priv->items, _item);
    }

  /* Prepend item to start of the history */
  history->priv->items = g_slist_prepend (history->priv->items, item);
  history->priv->item_to_restore = item;

  /* Emit signal */
  g_signal_emit (history, signals[ITEM_ADDED], 0);
}

/*
 * Misc functions
 */

static void
__clipman_history_item_free (ClipmanHistoryItem *item)
{
  switch (item->type)
    {
    case CLIPMAN_HISTORY_TYPE_TEXT:
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
__g_slist_compare_texts (gconstpointer a,
                         gconstpointer b)
{
  const ClipmanHistoryItem *item = a;
  const gchar *text = b;
  if (item->type != CLIPMAN_HISTORY_TYPE_TEXT)
    return -1;
  return g_ascii_strcasecmp (item->content.text, text);
}

/*
 * Public methods
 */

/**
 * clipman_history_add_text:
 * @history:    a #ClipmanHistory
 * @text:       the text to add to the history
 *
 * Stores a text inside the history.  If the history is growing over the
 * maximum number of items, it will delete the oldest text.
 */
void
clipman_history_add_text (ClipmanHistory *history,
                          const gchar *text)
{
  ClipmanHistoryItem *item;
  gchar *tmp1, *tmp2;
  const gchar *offset;
  gint preview_length = 48;
  GSList *list;

  /* Search for a previously existing content */
  list = g_slist_find_custom (history->priv->items, text, (GCompareFunc)__g_slist_compare_texts);
  if (list != NULL)
    {
      DBG ("Found a previous occurence for text `%s'", text);
      item = list->data;
      if (history->priv->reorder_items)
        {
          __clipman_history_item_free (item);
          history->priv->items = g_slist_remove (history->priv->items, list->data);
        }
      else
        {
          history->priv->item_to_restore = item;
          return;
        }
    }

  /* Store the text */
  DBG ("Store text `%s')", text);

  item = g_slice_new0 (ClipmanHistoryItem);
  item->type = CLIPMAN_HISTORY_TYPE_TEXT;
  item->content.text = g_strdup (text);

  /* Strip white spaces for preview */
  tmp2 = tmp1 = g_strdup (text);

  g_strchug (tmp2);

  tmp2 = g_strstr_len (tmp2, preview_length, "  ");
  while (tmp2)
    {
      g_strchug (++tmp2);
      /* We've already parsed `tmp2 - tmp1` chars */
      tmp2 = g_strstr_len (tmp2, preview_length - (tmp2 - tmp1), "  ");
    }

  /* Shorten preview */
  if (g_utf8_strlen (tmp1, -1) > preview_length)
    {
      offset = g_utf8_offset_to_pointer (tmp1, preview_length);
      tmp2 = g_strndup (tmp1, offset - tmp1);
      g_free (tmp1);
      g_strchomp (tmp2);

      tmp1 = g_strconcat (tmp2, "...", NULL);
      g_free (tmp2);
    }
  else
    g_strchomp (tmp1);

  /* Cleanup special characters from preview */
  g_strdelimit (tmp1, "\n\r\t", ' ');

  /* Set preview */
  item->preview.text = tmp1;

  _clipman_history_add_item (history, item);
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
 * Returns: a newly allocated #GSList that must be freed with g_slist_free()
 */
GSList *
clipman_history_get_list (ClipmanHistory *history)
{
  return g_slist_copy (history->priv->items);
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
 *
 * Clears the lists containing the history of the texts and the images.
 */
void
clipman_history_clear (ClipmanHistory *history)
{
  GSList *list;

  DBG ("Clear the history");

  for (list = history->priv->items; list != NULL; list = list->next)
    __clipman_history_item_free (list->data);

  g_slist_free (history->priv->items);
  history->priv->items = NULL;
  history->priv->item_to_restore = NULL;

  g_signal_emit (history, signals[CLEAR], 0);
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

static void
clipman_history_init (ClipmanHistory *history)
{
  history->priv = clipman_history_get_instance_private (history);
  history->priv->item_to_restore = NULL;
}

static void
clipman_history_finalize (GObject *object)
{
  clipman_history_clear (CLIPMAN_HISTORY (object));
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
