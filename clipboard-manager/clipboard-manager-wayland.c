/*
 * Copyright (C) 2023 Gaël Bonithon <gael@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-unix.h>
#include <gio/gunixinputstream.h>
#include <gdk/gdkwayland.h>
#include <gtk/gtk.h>

#include <protocols/wlr-data-control-unstable-v1-client.h>

#include "clipboard-manager-wayland.h"



static void xcp_clipboard_manager_wayland_finalize (GObject *object);

static void registry_global (void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void registry_global_remove (void *data, struct wl_registry *registry, uint32_t id);
static void device_data_offer (void *data, struct zwlr_data_control_device_v1 *device, struct zwlr_data_control_offer_v1 *offer);
static void device_selection (void *data, struct zwlr_data_control_device_v1 *device, struct zwlr_data_control_offer_v1 *offer);
static void device_finished (void *data, struct zwlr_data_control_device_v1 *device);
static void device_primary_selection (void *data, struct zwlr_data_control_device_v1 *device, struct zwlr_data_control_offer_v1 *offer);
static void offer_offer (void *data, struct zwlr_data_control_offer_v1 *offer, const char *mime_type);



#define BUFFER_SIZE 4096

typedef enum
{
  CLIPBOARD_TYPE_DEFAULT,
  CLIPBOARD_TYPE_PRIMARY,
  N_CLIPBOARD_TYPES,
} XcpClipboardType;

typedef enum
{
  DATA_TYPE_NONE = -1,
  DATA_TYPE_TEXT,
  DATA_TYPE_IMAGE,
  N_DATA_TYPES,
} XcpDataType;

struct _XcpClipboardManagerWayland
{
  GObject parent;

  struct wl_registry *wl_registry;
  struct zwlr_data_control_manager_v1 *wl_manager;
  struct zwlr_data_control_device_v1 *wl_device;

  GtkClipboard *clipboards[N_CLIPBOARD_TYPES];
  GCancellable *cancellables[N_CLIPBOARD_TYPES];
  gboolean own_changes[N_CLIPBOARD_TYPES];
  gchar *mime_type;
  XcpDataType data_type;
};

typedef struct _XcpLoadData
{
  struct zwlr_data_control_offer_v1 *wl_offer;
  XcpClipboardManagerWayland *manager;
  XcpClipboardType clipboard_type;
  XcpDataType data_type;
  guchar buffer[BUFFER_SIZE + 1];
  gchar *text;
} XcpLoadData;

static const struct wl_registry_listener registry_listener =
{
  .global = registry_global,
  .global_remove = registry_global_remove,
};

static const struct zwlr_data_control_device_v1_listener device_listener =
{
  .data_offer = device_data_offer,
  .selection = device_selection,
  .finished = device_finished,
  .primary_selection = device_primary_selection,
};

static const struct zwlr_data_control_offer_v1_listener offer_listener =
{
  .offer = offer_offer,
};



G_DEFINE_TYPE (XcpClipboardManagerWayland, xcp_clipboard_manager_wayland, G_TYPE_OBJECT)



static void
xcp_clipboard_manager_wayland_class_init (XcpClipboardManagerWaylandClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xcp_clipboard_manager_wayland_finalize;
}



static void
xcp_clipboard_manager_wayland_init (XcpClipboardManagerWayland *manager)
{
  GdkDisplay *display = gdk_display_get_default ();
  struct wl_display *wl_display = gdk_wayland_display_get_wl_display (display);

  manager->wl_registry = wl_display_get_registry (wl_display);
  wl_registry_add_listener (manager->wl_registry, &registry_listener, manager);
  wl_display_roundtrip (wl_display);
  if (manager->wl_manager != NULL)
    {
      struct wl_seat *wl_seat = gdk_wayland_seat_get_wl_seat (gdk_display_get_default_seat (display));
      manager->wl_device = zwlr_data_control_manager_v1_get_data_device (manager->wl_manager, wl_seat);
      zwlr_data_control_device_v1_add_listener (manager->wl_device, &device_listener, manager);

      manager->clipboards[CLIPBOARD_TYPE_DEFAULT] = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
      manager->clipboards[CLIPBOARD_TYPE_PRIMARY] = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
    }
  else
    g_warning ("Your compositor does not seem to support wlr-data-control protocol:"
               " most of Clipman's features won't work");
}



static void
xcp_clipboard_manager_wayland_finalize (GObject *object)
{
  XcpClipboardManagerWayland *manager = XCP_CLIPBOARD_MANAGER_WAYLAND (object);

  for (gint i = 0; i < N_CLIPBOARD_TYPES; i++)
    g_cancellable_cancel (manager->cancellables[i]);

  g_free (manager->mime_type);

  if (manager->wl_device != NULL)
    zwlr_data_control_device_v1_destroy (manager->wl_device);
  if (manager->wl_manager != NULL)
    zwlr_data_control_manager_v1_destroy (manager->wl_manager);
  wl_registry_destroy (manager->wl_registry);

  G_OBJECT_CLASS (xcp_clipboard_manager_wayland_parent_class)->finalize (object);
}



static void
registry_global (void *data,
                 struct wl_registry *registry,
                 uint32_t id,
                 const char *interface,
                 uint32_t version)
{
  XcpClipboardManagerWayland *manager = data;

  if (strcmp (zwlr_data_control_manager_v1_interface.name, interface) == 0)
    manager->wl_manager = wl_registry_bind (manager->wl_registry, id, &zwlr_data_control_manager_v1_interface,
                                            MIN ((uint32_t) zwlr_data_control_manager_v1_interface.version, version));
}



static void
registry_global_remove (void *data,
                        struct wl_registry *registry,
                        uint32_t id)
{
}



static void
device_data_offer (void *data,
                   struct zwlr_data_control_device_v1 *device,
                   struct zwlr_data_control_offer_v1 *offer)
{
  XcpClipboardManagerWayland *manager = data;

  zwlr_data_control_offer_v1_add_listener (offer, &offer_listener, data);
  manager->data_type = DATA_TYPE_NONE;
}



static void
offer_destroy_load_data (XcpLoadData *load_data)
{
  zwlr_data_control_offer_v1_destroy (load_data->wl_offer);
  g_free (load_data->text);
  load_data->manager->cancellables[load_data->clipboard_type] = NULL;
  g_free (load_data);
}



static void
offer_request_image (GObject *source_object,
                     GAsyncResult *res,
                     gpointer data)
{
  XcpLoadData *load_data = data;
  GtkClipboard *clipboard = load_data->manager->clipboards[load_data->clipboard_type];
  GError *error = NULL;
  GdkPixbuf *image;

  image = gdk_pixbuf_new_from_stream_finish (res, &error);
  if (image == NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning ("Failed to get image from pipe: %s", error->message);
      g_error_free (error);
      offer_destroy_load_data (load_data);
      return;
    }

  load_data->manager->own_changes[load_data->clipboard_type] = TRUE;
  gtk_clipboard_set_image (clipboard, image);
  wl_display_roundtrip (gdk_wayland_display_get_wl_display (gdk_display_get_default ()));
  load_data->manager->own_changes[load_data->clipboard_type] = FALSE;
  g_signal_emit_by_name (clipboard, "owner-change", NULL);

  g_object_unref (image);
  offer_destroy_load_data (load_data);
}



static void
offer_request_text (GObject *source_object,
                    GAsyncResult *res,
                    gpointer data)
{
  XcpLoadData *load_data = data;
  GtkClipboard *clipboard = load_data->manager->clipboards[load_data->clipboard_type];
  GInputStream *stream = G_INPUT_STREAM (source_object);
  GError *error = NULL;
  gssize size = g_input_stream_read_finish (stream, res, &error);

  if (size == -1)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning ("Failed to get text from pipe: %s", error->message);
      g_error_free (error);
      offer_destroy_load_data (load_data);
      return;
    }

  if (size > 0)
    {
      gchar *text;
      load_data->buffer[size] = '\0';
      text = g_strconcat (load_data->text, load_data->buffer, NULL);
      g_free (load_data->text);
      load_data->text = text;
      g_input_stream_read_async (stream, load_data->buffer, BUFFER_SIZE, G_PRIORITY_DEFAULT,
                                 load_data->manager->cancellables[load_data->clipboard_type],
                                 offer_request_text, load_data);
      return;
    }

  load_data->manager->own_changes[load_data->clipboard_type] = TRUE;
  gtk_clipboard_set_text (clipboard, load_data->text, -1);
  wl_display_roundtrip (gdk_wayland_display_get_wl_display (gdk_display_get_default ()));
  load_data->manager->own_changes[load_data->clipboard_type] = FALSE;
  g_signal_emit_by_name (clipboard, "owner-change", NULL);

  offer_destroy_load_data (load_data);
}



static gboolean
offer_request_data (struct zwlr_data_control_offer_v1 *offer,
                    XcpClipboardManagerWayland *manager,
                    XcpClipboardType clipboard_type)
{
  XcpLoadData *load_data;
  GInputStream *stream;
  GCancellable *cancellable;
  GError *error = NULL;
  gint fds[2];
  gint flags = 0;
#if GLIB_CHECK_VERSION (2, 77, 0)
  flags |= O_NONBLOCK;
#endif

  /*
   * Setting O_NONBLOCK flag *and* using async stream methods below is required to avoid
   * any blocking of the plugin, here or later when loading data.
   * The GIOChannel API could also be used without blocking, especially for DATA_TYPE_TEXT,
   * but final data loading often fails, unlike with the GIOInputStream API.
   */
  if (!g_unix_open_pipe (fds, flags, &error))
    {
      g_warning ("Failed to open pipe: %s", error->message);
      g_error_free (error);
      return FALSE;
    }

#if !GLIB_CHECK_VERSION (2, 77, 0)
  if (!g_unix_set_fd_nonblocking (fds[0], TRUE, &error) || !g_unix_set_fd_nonblocking (fds[1], TRUE, &error))
    {
      g_warning ("Failed to set nonblock flag on pipe: %s", error->message);
      g_error_free (error);
      close (fds[0]);
      close (fds[1]);
      return FALSE;
    }
#endif

  zwlr_data_control_offer_v1_receive (offer, manager->mime_type, fds[1]);
  wl_display_flush (gdk_wayland_display_get_wl_display (gdk_display_get_default ()));
  close (fds[1]);

  stream = g_unix_input_stream_new (fds[0], TRUE);
  cancellable = g_cancellable_new ();
  manager->cancellables[clipboard_type] = cancellable;

  load_data = g_new0 (XcpLoadData, 1);
  load_data->wl_offer = offer;
  load_data->manager = manager;
  load_data->clipboard_type = clipboard_type;
  load_data->data_type = manager->data_type;

  if (load_data->data_type == DATA_TYPE_TEXT)
    {
      load_data->text = g_strdup ("");
      g_input_stream_read_async (stream, load_data->buffer, BUFFER_SIZE, G_PRIORITY_DEFAULT,
                                 cancellable, offer_request_text, load_data);
    }
  else if (load_data->data_type == DATA_TYPE_IMAGE)
    gdk_pixbuf_new_from_stream_async (stream, cancellable, offer_request_image, load_data);

  g_object_unref (stream);
  g_object_unref (cancellable);

  return TRUE;
}



static void
device_selection (void *data,
                  struct zwlr_data_control_device_v1 *device,
                  struct zwlr_data_control_offer_v1 *offer)
{
  XcpClipboardManagerWayland *manager = data;

  g_cancellable_cancel (manager->cancellables[CLIPBOARD_TYPE_DEFAULT]);

  if (offer == NULL)
    return;

  if (manager->own_changes[CLIPBOARD_TYPE_DEFAULT] || manager->data_type == DATA_TYPE_NONE)
    {
      zwlr_data_control_offer_v1_destroy (offer);
      return;
    }

  if (!offer_request_data (offer, manager, CLIPBOARD_TYPE_DEFAULT))
    zwlr_data_control_offer_v1_destroy (offer);
}



static void
device_finished (void *data,
                 struct zwlr_data_control_device_v1 *device)
{
  XcpClipboardManagerWayland *manager = data;

  zwlr_data_control_device_v1_destroy (manager->wl_device);
  manager->wl_device = NULL;
}



static void
device_primary_selection (void *data,
                          struct zwlr_data_control_device_v1 *device,
                          struct zwlr_data_control_offer_v1 *offer)
{
  XcpClipboardManagerWayland *manager = data;

  g_cancellable_cancel (manager->cancellables[CLIPBOARD_TYPE_PRIMARY]);

  if (offer == NULL)
    return;

  if (manager->own_changes[CLIPBOARD_TYPE_PRIMARY] || manager->data_type == DATA_TYPE_NONE)
    {
      zwlr_data_control_offer_v1_destroy (offer);
      return;
    }

  if (!offer_request_data (offer, manager, CLIPBOARD_TYPE_PRIMARY))
    zwlr_data_control_offer_v1_destroy (offer);
}



static void
offer_offer (void *data,
             struct zwlr_data_control_offer_v1 *offer,
             const char *mime_type)
{
  XcpClipboardManagerWayland *manager = data;

  /* we already match */
  if (manager->data_type != DATA_TYPE_NONE)
    return;

  if (strcmp (mime_type, "text/plain;charset=utf-8") == 0)
    manager->data_type = DATA_TYPE_TEXT;
  else if (strcmp (mime_type, "image/png") == 0)
    manager->data_type = DATA_TYPE_IMAGE;

  if (manager->data_type != DATA_TYPE_NONE)
    {
      g_free (manager->mime_type);
      manager->mime_type = g_strdup (mime_type);
    }
}
