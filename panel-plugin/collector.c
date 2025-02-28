/*
 *  Copyright (c) 2008-2012 Mike Massonnet <mmassonnet@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "actions.h"
#include "collector.h"
#include "common.h"
#include "history.h"

#include <libxfce4util/libxfce4util.h>

/*
 * GObject declarations
 */

struct _ClipmanCollectorPrivate
{
  ClipmanActions *actions;
  ClipmanHistory *history;
  GtkClipboard *default_clipboard;
  GtkClipboard *primary_clipboard;
  GdkPixbuf *current_image;
  gchar *default_cache;
  gchar *primary_cache;
  guint primary_clipboard_timeout;
  gboolean default_internal_change;
  gboolean primary_internal_change;
  gboolean add_primary_clipboard;
  gboolean persistent_primary_clipboard;
  gboolean history_ignore_primary_clipboard;
  gboolean enable_actions;
  gboolean inhibit;
};

G_DEFINE_TYPE_WITH_PRIVATE (ClipmanCollector, clipman_collector, G_TYPE_OBJECT)

enum
{
  ADD_PRIMARY_CLIPBOARD = 1,
  PERSISTENT_PRIMARY_CLIPBOARD,
  HISTORY_IGNORE_PRIMARY_CLIPBOARD,
  ENABLE_ACTIONS,
  INHIBIT,
};

static void
clipman_collector_constructed (GObject *object);
static void
clipman_collector_finalize (GObject *object);
static void
clipman_collector_set_property (GObject *object,
                                guint property_id,
                                const GValue *value,
                                GParamSpec *pspec);
static void
clipman_collector_get_property (GObject *object,
                                guint property_id,
                                GValue *value,
                                GParamSpec *pspec);

/*
 * Callbacks declarations
 */

static void
cb_clipboard_owner_change (ClipmanCollector *collector,
                           GdkEventOwnerChange *event,
                           GtkClipboard *clipboard);
static void
cb_request_text (GtkClipboard *clipboard,
                 const gchar *text,
                 ClipmanCollector *collector);



/*
 * Callbacks
 */
static gboolean
cb_check_primary_clipboard (gpointer user_data)
{
  ClipmanCollector *collector = user_data;
  GdkModifierType state = 0;
  GdkDisplay *display = gdk_display_get_default ();
  GdkSeat *seat = gdk_display_get_default_seat (display);
  GdkDevice *device = gdk_seat_get_pointer (seat);
  GdkScreen *screen = gdk_screen_get_default ();
  GdkWindow *root_win = gdk_screen_get_root_window (screen);

  g_return_val_if_fail (GTK_IS_CLIPBOARD (collector->priv->default_clipboard) && GTK_IS_CLIPBOARD (collector->priv->primary_clipboard), FALSE);

  /* Jump over if the content is set from within clipman */
  if (collector->priv->primary_internal_change)
    {
      collector->priv->primary_internal_change = FALSE;
      collector->priv->primary_clipboard_timeout = 0;
      return FALSE;
    }

  /* Postpone until the selection is done */
  gdk_window_get_device_position (root_win, device, NULL, NULL, &state);
  if (state & (GDK_BUTTON1_MASK | GDK_SHIFT_MASK))
    return TRUE;

  gtk_clipboard_request_text (collector->priv->primary_clipboard,
                              (GtkClipboardTextReceivedFunc) cb_request_text,
                              collector);

  collector->priv->primary_clipboard_timeout = 0;
  return FALSE;
}


static void
cb_clipboard_owner_change (ClipmanCollector *collector,
                           GdkEventOwnerChange *event,
                           GtkClipboard *clipboard)
{
  g_return_if_fail (GTK_IS_CLIPBOARD (collector->priv->default_clipboard) && GTK_IS_CLIPBOARD (collector->priv->primary_clipboard));

  /* Jump over if collector is inhibited */
  if (collector->priv->inhibit)
    {
      return;
    }

  /* We're only interested in the signals we send ourselves from the clipboard manager on Wayland.
   * GTK signals are duplicative and can cause an infinite loop when showing the action menu
   * (by losing then regaining focus). */
  if (event != NULL && WINDOWING_IS_WAYLAND ())
    {
      return;
    }

  /* Save the clipboard content to ClipmanHistory */
  if (clipboard == collector->priv->default_clipboard)
    {
      /* Jump over if the content is set from within clipman */
      if (collector->priv->default_internal_change)
        {
          collector->priv->default_internal_change = FALSE;
          return;
        }

      g_clear_object (&collector->priv->current_image);
      if (gtk_clipboard_wait_is_image_available (collector->priv->default_clipboard))
        {
          GdkPixbuf *image;

          /* first clear default cache, so we don't restore it while waiting */
          g_free (collector->priv->default_cache);
          collector->priv->default_cache = NULL;

          image = gtk_clipboard_wait_for_image (collector->priv->default_clipboard);
          if (image != NULL)
            {
              collector->priv->current_image = image;
              clipman_history_add_image (collector->priv->history, image);
            }
        }
      else
        {
          gtk_clipboard_request_text (collector->priv->default_clipboard,
                                      (GtkClipboardTextReceivedFunc) cb_request_text,
                                      collector);
        }
    }
  else if (clipboard == collector->priv->primary_clipboard)
    {
      /* This clipboard is due to many changes while selecting, therefore we
       * actually check inside a delayed timeout if the mouse is still pressed
       * or if the shift key is hold down, and once both are released the
       * content will go to the history. */
      if (collector->priv->add_primary_clipboard
          || collector->priv->persistent_primary_clipboard
          || !collector->priv->history_ignore_primary_clipboard
          || collector->priv->enable_actions)
        {
          if (collector->priv->primary_clipboard_timeout != 0)
            {
              g_source_remove (collector->priv->primary_clipboard_timeout);
              collector->priv->primary_clipboard_timeout = 0;
            }
          collector->priv->primary_clipboard_timeout =
            g_timeout_add (250, cb_check_primary_clipboard, collector);
        }
    }
}

static void
cb_request_text (GtkClipboard *clipboard,
                 const gchar *text,
                 ClipmanCollector *collector)
{
  g_return_if_fail (GTK_IS_CLIPBOARD (collector->priv->default_clipboard) && GTK_IS_CLIPBOARD (collector->priv->primary_clipboard));

  if (text == NULL)
    {
      /* Restore primary clipboard on deselection */
      if (clipboard == collector->priv->primary_clipboard
          && collector->priv->primary_cache != NULL
          && ((collector->priv->persistent_primary_clipboard && !collector->priv->add_primary_clipboard)
              || (collector->priv->add_primary_clipboard
                  && gtk_clipboard_wait_is_text_available (collector->priv->default_clipboard))))
        {
          collector->priv->primary_internal_change = TRUE;
          gtk_clipboard_set_text (collector->priv->primary_clipboard, collector->priv->primary_cache, -1);
        }

      /* Restore default clipboard when cleared unintentionally (if this was intentional,
       * 'default_cache' should have been cleared before) */
      if (clipboard == collector->priv->default_clipboard && collector->priv->default_cache != NULL)
        {
          GdkAtom *targets;
          gint n_targets;
          if (!gtk_clipboard_wait_for_targets (collector->priv->default_clipboard, &targets, &n_targets))
            {
              collector->priv->default_internal_change = TRUE;
              gtk_clipboard_set_text (collector->priv->default_clipboard, collector->priv->default_cache, -1);
            }
          else
            {
              /* text was reset but clipboard contains some data (e.g. objects in FreeCAD,
               * see https://gitlab.xfce.org/panel-plugins/xfce4-clipman-plugin/-/issues/85) */
              g_free (targets);
            }
        }

      return;
    }

  if (clipboard == collector->priv->default_clipboard)
    {
      /* Add to history */
      clipman_history_add_text (collector->priv->history, text);

      /* Make a copy inside the primary clipboard */
      if (collector->priv->add_primary_clipboard && g_strcmp0 (text, collector->priv->primary_cache) != 0)
        {
          collector->priv->primary_internal_change = TRUE;
          gtk_clipboard_set_text (collector->priv->primary_clipboard, text, -1);
          g_free (collector->priv->primary_cache);
          collector->priv->primary_cache = g_strdup (text);
        }

      /* Match for actions */
      if (collector->priv->enable_actions)
        clipman_actions_match_with_menu (collector->priv->actions, ACTION_GROUP_MANUAL, text);

      g_free (collector->priv->default_cache);
      collector->priv->default_cache = g_strdup (text);
    }
  else if (clipboard == collector->priv->primary_clipboard)
    {
      /* Avoid history */
      if (!collector->priv->history_ignore_primary_clipboard)
        clipman_history_add_text (collector->priv->history, text);

      /* Make a copy inside the default clipboard */
      if (collector->priv->add_primary_clipboard && g_strcmp0 (text, collector->priv->default_cache) != 0)
        {
          collector->priv->default_internal_change = TRUE;
          gtk_clipboard_set_text (collector->priv->default_clipboard, text, -1);
          g_free (collector->priv->default_cache);
          collector->priv->default_cache = g_strdup (text);
        }

      /* Match for actions */
      if (collector->priv->enable_actions)
        clipman_actions_match_with_menu (collector->priv->actions, ACTION_GROUP_SELECTION, text);

      /* Store selection for later use */
      if (collector->priv->persistent_primary_clipboard || collector->priv->add_primary_clipboard)
        {
          g_free (collector->priv->primary_cache);
          collector->priv->primary_cache = g_strdup (text);
        }
    }
}

/*
 * Public methods
 */

/**
 * clipman_collector_set_is_restoring:
 * @collector: a #ClipmanCollector
 *
 * Call this function before modifying the content of a #GtkClipboard so that
 * the new content won't be looked by #ClipmanCollector.
 */
void
clipman_collector_set_is_restoring (ClipmanCollector *collector,
                                    GtkClipboard *clipboard)
{
  if (clipboard == collector->priv->default_clipboard)
    collector->priv->default_internal_change = TRUE;
  else if (clipboard == collector->priv->primary_clipboard)
    collector->priv->primary_internal_change = TRUE;
}

void
clipman_collector_clear_cache (ClipmanCollector *collector)
{
  g_clear_object (&collector->priv->current_image);
  g_free (collector->priv->default_cache);
  g_free (collector->priv->primary_cache);
  collector->priv->default_cache = NULL;
  collector->priv->primary_cache = NULL;
}

GdkPixbuf *
clipman_collector_get_current_image (ClipmanCollector *collector)
{
  return collector->priv->current_image;
}

ClipmanCollector *
clipman_collector_get (void)
{
  static ClipmanCollector *singleton = NULL;

  if (singleton == NULL)
    {
      singleton = g_object_new (CLIPMAN_TYPE_COLLECTOR, NULL);
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
clipman_collector_class_init (ClipmanCollectorClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->constructed = clipman_collector_constructed;
  object_class->finalize = clipman_collector_finalize;
  object_class->set_property = clipman_collector_set_property;
  object_class->get_property = clipman_collector_get_property;

  g_object_class_install_property (object_class, ADD_PRIMARY_CLIPBOARD,
                                   g_param_spec_boolean ("add-primary-clipboard",
                                                         "AddPrimaryClipboard",
                                                         "Sync the primary clipboard with the default clipboard",
                                                         DEFAULT_ADD_PRIMARY_CLIPBOARD,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PERSISTENT_PRIMARY_CLIPBOARD,
                                   g_param_spec_boolean ("persistent-primary-clipboard",
                                                         "PersistentPrimaryClipboard",
                                                         "Make the primary clipboard persistent over deselection",
                                                         DEFAULT_PERSISTENT_PRIMARY_CLIPBOARD,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, HISTORY_IGNORE_PRIMARY_CLIPBOARD,
                                   g_param_spec_boolean ("history-ignore-primary-clipboard",
                                                         "HistoryIgnorePrimaryClipboard",
                                                         "Exclude the primary clipboard contents from the history",
                                                         DEFAULT_HISTORY_IGNORE_PRIMARY_CLIPBOARD,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, ENABLE_ACTIONS,
                                   g_param_spec_boolean ("enable-actions",
                                                         "EnableActions",
                                                         "Set to TRUE to enable actions (match the clipboard texts against regex's)",
                                                         DEFAULT_ENABLE_ACTIONS,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, INHIBIT,
                                   g_param_spec_boolean ("inhibit",
                                                         "Inhibit",
                                                         "Set to TRUE to disable the collector",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
clipman_collector_init (ClipmanCollector *collector)
{
  collector->priv = clipman_collector_get_instance_private (collector);

  /* This bit is set to TRUE when a clipboard is set from within clipman to avoid
   * re-adding to the history or triggering actions several times */
  collector->priv->default_internal_change = FALSE;
  collector->priv->primary_internal_change = FALSE;

  /* ClipmanActions */
  collector->priv->actions = clipman_actions_get ();

  /* ClipmanHistory */
  collector->priv->history = clipman_history_get ();

  /* Clipboards */
  collector->priv->default_clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  collector->priv->primary_clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
}

static void
clipman_collector_constructed (GObject *object)
{
  ClipmanCollector *collector = CLIPMAN_COLLECTOR (object);

  g_signal_connect_object (collector->priv->default_clipboard, "owner-change",
                           G_CALLBACK (cb_clipboard_owner_change), collector, G_CONNECT_SWAPPED);
  g_signal_connect_object (collector->priv->primary_clipboard, "owner-change",
                           G_CALLBACK (cb_clipboard_owner_change), collector, G_CONNECT_SWAPPED);

  /* initialize image cache for proper detection at startup in the menu */
  collector->priv->current_image = gtk_clipboard_wait_for_image (collector->priv->default_clipboard);
}

static void
clipman_collector_finalize (GObject *object)
{
  ClipmanCollector *collector = CLIPMAN_COLLECTOR (object);

  if (collector->priv->primary_clipboard_timeout != 0)
    {
      g_source_remove (collector->priv->primary_clipboard_timeout);
      collector->priv->primary_clipboard_timeout = 0;
    }
  g_object_unref (collector->priv->actions);
  g_object_unref (collector->priv->history);
  clipman_collector_clear_cache (collector);

  G_OBJECT_CLASS (clipman_collector_parent_class)->finalize (object);
}

static void
clipman_collector_set_property (GObject *object,
                                guint property_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
  ClipmanCollectorPrivate *priv = CLIPMAN_COLLECTOR (object)->priv;

  switch (property_id)
    {
    case ADD_PRIMARY_CLIPBOARD:
      priv->add_primary_clipboard = g_value_get_boolean (value);
      break;

    case PERSISTENT_PRIMARY_CLIPBOARD:
      priv->persistent_primary_clipboard = g_value_get_boolean (value);
      break;

    case HISTORY_IGNORE_PRIMARY_CLIPBOARD:
      priv->history_ignore_primary_clipboard = g_value_get_boolean (value);
      break;

    case ENABLE_ACTIONS:
      priv->enable_actions = g_value_get_boolean (value);
      break;

    case INHIBIT:
      priv->inhibit = g_value_get_boolean (value);
      break;

    default:
      break;
    }
}

static void
clipman_collector_get_property (GObject *object,
                                guint property_id,
                                GValue *value,
                                GParamSpec *pspec)
{
  ClipmanCollectorPrivate *priv = CLIPMAN_COLLECTOR (object)->priv;

  switch (property_id)
    {
    case ADD_PRIMARY_CLIPBOARD:
      g_value_set_boolean (value, priv->add_primary_clipboard);
      break;

    case PERSISTENT_PRIMARY_CLIPBOARD:
      g_value_set_boolean (value, priv->persistent_primary_clipboard);
      break;

    case HISTORY_IGNORE_PRIMARY_CLIPBOARD:
      g_value_set_boolean (value, priv->history_ignore_primary_clipboard);
      break;

    case ENABLE_ACTIONS:
      g_value_set_boolean (value, priv->enable_actions);
      break;

    case INHIBIT:
      g_value_set_boolean (value, priv->inhibit);
      break;

    default:
      break;
    }
}
