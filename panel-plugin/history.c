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
#include "config.h"
#endif

#include "common.h"
#include "history.h"

#include <glib/gstdio.h>
#include <libxfce4util/libxfce4util.h>

/*
 * GObject declarations
 */

struct _ClipmanHistoryPrivate
{
  GSList *items;
  guint max_texts_in_history;
  guint max_images_in_history;
  gboolean save_on_quit;
  gboolean reorder_items;
  gint scale_factor;
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

static void
clipman_history_finalize (GObject *object);
static void
clipman_history_set_property (GObject *object,
                              guint property_id,
                              const GValue *value,
                              GParamSpec *pspec);
static void
clipman_history_get_property (GObject *object,
                              guint property_id,
                              GValue *value,
                              GParamSpec *pspec);

/*
 * Private methods declarations
 */

static void
_clipman_history_add_item (ClipmanHistory *history,
                           ClipmanHistoryItem *item);

/*
 * Misc functions declarations
 */

static void
__clipman_history_item_free (ClipmanHistoryItem *item);
static gint
__g_slist_compare_texts (gconstpointer a,
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
  guint n_images = 0;

  /* we're going to work from last item */
  history->priv->items = g_slist_reverse (history->priv->items);

  /* count images */
  for (list = history->priv->items; list != NULL; list = list->next)
    {
      _item = list->data;
      if (_item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
        {
          n_images++;
        }
    }

  if (item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
    n_images++;

  /* free last images from history if max_images is reached */
  list = history->priv->items;
  while (n_images > history->priv->max_images_in_history)
    {
      GSList *next = g_slist_next (list);
      DBG ("Delete oldest images from the history");
      _item = list->data;
      if (_item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
        {
          __clipman_history_item_free (_item);
          history->priv->items = g_slist_delete_link (history->priv->items, list);
          n_images--;
        }
      list = next;
    }

  list_length = g_slist_length (history->priv->items) + 1;

  /* truncate history to max_items (max_texts stands for the size of the history) */
  while (list_length > history->priv->max_texts_in_history)
    {
      DBG ("Delete oldest contents from the history");
      _item = history->priv->items->data;
      __clipman_history_item_free (_item);
      history->priv->items = g_slist_delete_link (history->priv->items, history->priv->items);
      list_length--;
    }

  /* Prepend item to start of the history */
  history->priv->items = g_slist_reverse (history->priv->items);
  history->priv->items = g_slist_prepend (history->priv->items, item);

  /* Emit signal */
  g_signal_emit (history, signals[ITEM_ADDED], 0);
}

static void
_clipman_history_image_set_preview (ClipmanHistory *history,
                                    ClipmanHistoryItem *item)
{
  gint size;

  g_return_if_fail (item->type == CLIPMAN_HISTORY_TYPE_IMAGE);

  if (item->preview.image != NULL)
    g_object_unref (item->preview.image);

  size = 128 * history->priv->scale_factor;
  item->preview.image = gdk_pixbuf_scale_simple (item->content.image, size, size, GDK_INTERP_BILINEAR);
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
      g_bytes_unref (item->pixel_bytes);
      if (item->filename != NULL)
        {
          g_unlink (item->filename);
          g_free (item->filename);
        }
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
  return g_strcmp0 (item->content.text, text);
}

static gint
__g_slist_compare_images (gconstpointer a,
                          gconstpointer b)
{
  const ClipmanHistoryItem *item = a;
  const GBytes *pixel_bytes = b;
  if (item->type != CLIPMAN_HISTORY_TYPE_IMAGE)
    return -1;
  return g_bytes_compare (item->pixel_bytes, pixel_bytes);
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
  GSList *list;

  if (text == NULL || *text == '\0')
    return;

  /* Search for a previously existing content */
  list = g_slist_find_custom (history->priv->items, text, __g_slist_compare_texts);
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
          return;
        }
    }

  /* Store the text */
  DBG ("Store text `%s')", text);

  item = g_slice_new0 (ClipmanHistoryItem);
  item->type = CLIPMAN_HISTORY_TYPE_TEXT;
  item->content.text = g_strdup (text);

  /* Set preview */
  item->preview.text = clipman_common_shorten_preview (text);

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
ClipmanHistoryItem *
clipman_history_add_image (ClipmanHistory *history,
                           const GdkPixbuf *image)
{
  ClipmanHistoryItem *item;
  GBytes *pixel_bytes;
  GSList *list;

  if (history->priv->max_images_in_history == 0)
    return NULL;

  /* Search for a previously existing content */
  pixel_bytes = gdk_pixbuf_read_pixel_bytes (image);
  list = g_slist_find_custom (history->priv->items, pixel_bytes, __g_slist_compare_images);
  g_bytes_unref (pixel_bytes);
  if (list != NULL)
    {
      DBG ("Found a previous occurence for image (%p)", image);
      item = list->data;
      if (history->priv->reorder_items)
        {
          history->priv->items = g_slist_remove (history->priv->items, item);
          history->priv->items = g_slist_prepend (history->priv->items, item);
          g_signal_emit (history, signals[ITEM_ADDED], 0);
        }
      return NULL;
    }

  DBG ("Store image (%p)", image);

  item = g_slice_new0 (ClipmanHistoryItem);
  item->type = CLIPMAN_HISTORY_TYPE_IMAGE;
  item->content.image = gdk_pixbuf_copy (image);
  item->pixel_bytes = gdk_pixbuf_read_pixel_bytes (item->content.image);
  _clipman_history_image_set_preview (history, item);

  DBG ("Copy of image (%p) is (%p)", image, item->content.image);

  _clipman_history_add_item (history, item);

  return item;
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

guint
clipman_history_get_max_texts_in_history (ClipmanHistory *history)
{
  return history->priv->max_texts_in_history;
}

guint
clipman_history_get_max_images_in_history (ClipmanHistory *history)
{
  return history->priv->max_images_in_history;
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
  DBG ("Clear the history");

  g_slist_free_full (history->priv->items, (GDestroyNotify) __clipman_history_item_free);
  history->priv->items = NULL;

  g_signal_emit (history, signals[CLEAR], 0);
}

void
clipman_history_set_scale_factor (ClipmanHistory *history,
                                  GParamSpec *pspec,
                                  GtkWidget *widget)
{
  gint scale_factor;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  scale_factor = gtk_widget_get_scale_factor (widget);
  if (scale_factor == history->priv->scale_factor)
    return;

  history->priv->scale_factor = scale_factor;
  for (GSList *lp = history->priv->items; lp != NULL; lp = lp->next)
    {
      ClipmanHistoryItem *item = lp->data;
      if (item->type == CLIPMAN_HISTORY_TYPE_IMAGE)
        _clipman_history_image_set_preview (history, item);
    }
}

ClipmanHistory *
clipman_history_get (void)
{
  static ClipmanHistory *singleton = NULL;

  if (singleton == NULL)
    {
      singleton = g_object_new (CLIPMAN_TYPE_HISTORY, NULL);
      g_object_add_weak_pointer (G_OBJECT (singleton), (gpointer) &singleton);
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

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = clipman_history_finalize;
  object_class->set_property = clipman_history_set_property;
  object_class->get_property = clipman_history_get_property;

  signals[ITEM_ADDED] =
    g_signal_new ("item-added", G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ClipmanHistoryClass, item_added),
                  0, NULL, g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[CLEAR] =
    g_signal_new ("clear", G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (ClipmanHistoryClass, clear),
                  0, NULL, g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class,
                                   MAX_TEXTS_IN_HISTORY,
                                   g_param_spec_uint ("max-texts-in-history",
                                                      "MaxTextsInHistory",
                                                      "The number of maximum texts in history",
                                                      5, 1000, DEFAULT_MAX_TEXTS_IN_HISTORY,
                                                      G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   MAX_IMAGES_IN_HISTORY,
                                   g_param_spec_uint ("max-images-in-history",
                                                      "MaxImagesInHistory",
                                                      "The number of maximum images in history",
                                                      0, 5, DEFAULT_MAX_IMAGES_IN_HISTORY,
                                                      G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   SAVE_ON_QUIT,
                                   g_param_spec_boolean ("save-on-quit",
                                                         "SaveOnQuit",
                                                         "True if the history must be saved on quit",
                                                         DEFAULT_SAVE_ON_QUIT,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   REORDER_ITEMS,
                                   g_param_spec_boolean ("reorder-items",
                                                         "ReorderItems",
                                                         "Always push last clipboard content to the top of the history",
                                                         DEFAULT_REORDER_ITEMS,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
clipman_history_init (ClipmanHistory *history)
{
  history->priv = clipman_history_get_instance_private (history);
  history->priv->scale_factor = 1;
}

static void
clipman_history_finalize (GObject *object)
{
  ClipmanHistory *history = CLIPMAN_HISTORY (object);

  for (GSList *lp = history->priv->items; lp != NULL; lp = lp->next)
    {
      /* reset filenames now so image files are not deleted in clear() */
      ClipmanHistoryItem *item = lp->data;
      g_free (item->filename);
      item->filename = NULL;
    }
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
  guint old_value;

  switch (property_id)
    {
    case MAX_TEXTS_IN_HISTORY:
      old_value = priv->max_texts_in_history;
      priv->max_texts_in_history = g_value_get_uint (value);
      if (priv->items != NULL && priv->max_texts_in_history < old_value)
        {
          ClipmanHistoryItem *item = priv->items->data;
          priv->items = g_slist_delete_link (priv->items, priv->items);
          _clipman_history_add_item (CLIPMAN_HISTORY (object), item);
        }
      break;

    case MAX_IMAGES_IN_HISTORY:
      old_value = priv->max_images_in_history;
      priv->max_images_in_history = g_value_get_uint (value);
      if (priv->items != NULL && priv->max_images_in_history < old_value)
        {
          ClipmanHistoryItem *item = priv->items->data;
          priv->items = g_slist_delete_link (priv->items, priv->items);
          _clipman_history_add_item (CLIPMAN_HISTORY (object), item);
        }
      break;

    case SAVE_ON_QUIT:
      priv->save_on_quit = g_value_get_boolean (value);
      if (!priv->save_on_quit)
        clipman_history_clear (CLIPMAN_HISTORY (object));
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
