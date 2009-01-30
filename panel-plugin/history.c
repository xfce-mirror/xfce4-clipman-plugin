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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>

#include "common.h"

#include "history.h"

/*
 * GObject declarations
 */

G_DEFINE_TYPE (ClipmanHistory, clipman_history, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CLIPMAN_TYPE_HISTORY, ClipmanHistoryPrivate))

struct _ClipmanHistoryPrivate
{
  GSList               *texts;
  GSList               *images;
  const ClipmanHistoryItem *item_to_restore;
  guint                 max_texts_in_history;
  guint                 max_images_in_history;
  gboolean              save_on_quit;
};

enum
{
  MAX_TEXTS_IN_HISTORY = 1,
  MAX_IMAGES_IN_HISTORY,
  SAVE_ON_QUIT,
};

static void             clipman_history_class_init         (ClipmanHistoryClass *klass);
static void             clipman_history_init               (ClipmanHistory *history);
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
  GSList *list, *last;
  guint max_items;
  guint list_length;

  /* Pick up values */
  switch (item->type)
    {
    case CLIPMAN_HISTORY_TYPE_TEXT:
      list = history->priv->texts;
      max_items = history->priv->max_texts_in_history;
      break;

    case CLIPMAN_HISTORY_TYPE_IMAGE:
      list = history->priv->images;
      max_items = history->priv->max_images_in_history;
      break;

    default:
      g_assert_not_reached ();
    }

  /* Check if the history is full */
  list_length = g_slist_length (list);
  while (list_length >= max_items)
    {
      DBG ("Delete oldest content from the history");
      last = g_slist_last (list);
      __clipman_history_item_free (last->data);
      list = g_slist_delete_link (list, last);
      list_length--;
    }

  /* Add the new item to the history */
  list = g_slist_prepend (list, item);
  history->priv->item_to_restore = item;

  /* Update list pointer in private data */
  switch (item->type)
    {
    case CLIPMAN_HISTORY_TYPE_TEXT:
      history->priv->texts = list;
      break;

    case CLIPMAN_HISTORY_TYPE_IMAGE:
      history->priv->images = list;
      break;

    default:
      g_assert_not_reached ();
    }
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
  list = g_slist_find_custom (history->priv->texts, text, (GCompareFunc)__g_slist_compare_texts);
  if (list != NULL)
    {
      DBG ("Found a previous occurence for text `%s'", text);
      __clipman_history_item_free (list->data);
      history->priv->texts = g_slist_delete_link (history->priv->texts, list);
    }

  /* Store the text */
  DBG ("Store text `%s')", text);

  item = g_slice_new0 (ClipmanHistoryItem);
  item->type = CLIPMAN_HISTORY_TYPE_TEXT;
  item->content.text = g_strdup (text);

  /* Strip white spaces for preview */
  tmp1 = g_strstrip (g_strdup (text));
  while (g_strstr_len (tmp1, preview_length, "  "))
    {
      tmp2 = exo_str_replace (tmp1, "  ", " ");
      g_free (tmp1);
      tmp1 = tmp2;
    }

  /* Shorten preview */
  if (g_utf8_strlen (tmp1, -1) > preview_length)
    {
      offset = g_utf8_offset_to_pointer (tmp1, preview_length);
      tmp2 = g_strndup (tmp1, offset - tmp1);
      g_free (tmp1);
      tmp1 = g_strconcat (tmp2, "...", NULL);
      g_free (tmp2);
    }

  /* Cleanup special characters from preview */
  tmp1 = g_strdelimit (tmp1, "\n\r\t", ' ');

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
  item->preview.image = exo_gdk_pixbuf_scale_ratio (GDK_PIXBUF (image), 128);

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
  return g_slist_concat (g_slist_copy (history->priv->images), g_slist_copy (history->priv->texts));
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

  for (list = history->priv->texts; list != NULL; list = list->next)
    __clipman_history_item_free (list->data);

  for (list = history->priv->images; list != NULL; list = list->next)
    __clipman_history_item_free (list->data);

  g_slist_free (history->priv->texts);
  g_slist_free (history->priv->images);

  history->priv->texts = NULL;
  history->priv->images = NULL;
}

ClipmanHistory *
clipman_history_get ()
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

  g_type_class_add_private (klass, sizeof (ClipmanHistoryPrivate));

  clipman_history_parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = clipman_history_finalize;
  object_class->set_property = clipman_history_set_property;
  object_class->get_property = clipman_history_get_property;

  g_object_class_install_property (object_class,
                                   MAX_TEXTS_IN_HISTORY,
                                   g_param_spec_uint ("max-texts-in-history",
                                                      "MaxTextsInHistory",
                                                      "The number of maximum texts in history",
                                                      5, 100, DEFAULT_MAX_TEXTS_IN_HISTORY,
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
}

static void
clipman_history_init (ClipmanHistory *history)
{
  history->priv = GET_PRIVATE (history);
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

    default:
      break;
    }
}

