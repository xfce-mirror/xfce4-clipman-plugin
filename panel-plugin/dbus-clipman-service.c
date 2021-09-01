#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>

#include "history.h"
#include "dbus-clipman-service.h"
#include "secure_text.h"
#include "menu.h"
#include "collector.h"

static GDBusNodeInfo *clipman_dbus_introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const gchar clipman_dbus_introspection_xml[] =
  "<node>"
  "  <interface name='org.xfce.clipman.GDBus.service'>"
  "    <annotation name='org.gtk.GDBus.Annotation' value='This DBus Service is IPC control for clipman menu'/>"
  "    <annotation name='org.gtk.GDBus.Annotation' value='Status Draft'/>"
  "    <method name='list_history'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='return clipman history with ID'/>"
  "      <arg type='s' name='history_list_as_string' direction='out'/>"
  "    </method>"
  "    <method name='get_item_by_id'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='get an item from clipman history, will be decoded based on boolean value'/>"
  "      <arg type='b' name='decode_secure_text' direction='in'/>"
  "      <arg type='q' name='searched_id' direction='in'/>"
  "      <arg type='s' name='text_item_value' direction='out'/>"
  "    </method>"
  "    <method name='delete_item_by_id'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='delete an item from clipman history, from clipboard too'/>"
  "      <arg type='q' name='item_id' direction='in'/>"
  "      <arg type='b' name='result' direction='out'/>"
  "    </method>"
  "    <method name='add_item'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='add an new text item in clipman history, can be secured'/>"
  "      <arg type='b' name='secure' direction='in'/>"
  "      <arg type='s' name='value' direction='in'/>"
  "      <arg type='q' name='new_id' direction='out'/>"
  "    </method>"
  "    <method name='clear_history'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='clear clipman history, full or only secure text'/>"
  "      <arg type='b' name='clear_only_secure_text' direction='in'/>"
  "      <arg type='q' name='nb_element_cleared' direction='out'/>"
  "    </method>"
  "    <method name='set_secure_by_id'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='change secure text state based on boolean value'/>"
  "      <arg type='b' name='secure' direction='in'/>"
  "      <arg type='q' name='item_id' direction='in'/>"
  "      <arg type='b' name='result' direction='out'/>"
  "    </method>"
  "    <method name='collect_next_item_secure'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='clipman collector will set the next item(s) secure'/>"
  "      <arg type='q' name='nb_next_item_secured' direction='in'/>"
  "      <arg type='b' name='result' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static GVariant *
clipman_dbus_handle_get_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GError          **error,
                     gpointer          user_data)
{
  GVariant *ret;

  ret = NULL;

  return ret;
}

static gboolean
clipman_dbus_handle_set_property (GDBusConnection  *connection,
                     const gchar      *sender,
                     const gchar      *object_path,
                     const gchar      *interface_name,
                     const gchar      *property_name,
                     GVariant         *value,
                     GError          **error,
                     gpointer          user_data)
{
  return FALSE;
}

static gboolean
clipman_dbus_method_list_history (
                                  GVariant              *parameters,
                                  GDBusMethodInvocation *invocation)
{
  ClipmanHistory *history;
  GList *list, *l;
  ClipmanHistoryItem *item;

  history = clipman_history_get ();

  list = clipman_history_get_list (history);
  list = g_list_reverse (list);
  if (list == NULL)
    {
      // empty
      g_dbus_method_invocation_return_value (invocation,
                                             g_variant_new ("(s)", ""));
      return FALSE;
    }
  else
    {
      gchar *response = NULL, *tmp;
      char **split;
      gchar *text_content;

      for (l = list; l != NULL; l = l->next)
        {
          item = l->data;

          switch (item->type)
            {
            /* We ignore everything but text (no images or QR codes) */
            case CLIPMAN_HISTORY_TYPE_TEXT:
            case CLIPMAN_HISTORY_TYPE_SECURE_TEXT:
              if (item->type == CLIPMAN_HISTORY_TYPE_TEXT)
                {
                  // we currently handle newlines in clipboard content as ==> \n
                  // which is not revertable change, but enough for the PoC.
                  split          = g_strsplit(item->content.text, "\n", -1);
                  text_content   = g_strjoinv("\\n", split);
                  g_strfreev(split);
                }
              else
                {
                  // CLIPMAN_HISTORY_TYPE_SECURE_TEXT, don't disclose content send preview
                  text_content = g_strdup(item->preview.text);
                }

              if(response == NULL)
                {
                  response = g_strdup_printf ("%2d %s", item->id, text_content);
                }
              else
                {
                  tmp = g_strdup_printf ("%s\n%2d %s", response, item->id, text_content);
                  g_free (response);
                  response = tmp;
                }

              g_free(text_content);
              break;

            default:
              DBG("Ignoring non-text history type %d", item->type);
              continue;
            }
        }

      g_list_free (list);
      g_dbus_method_invocation_return_value (invocation,
                                             g_variant_new ("(s)", response));
      g_free (response);
      return TRUE;
    }
}

static gboolean
clipman_dbus_method_get_item_by_id(
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation)
{
  guint searched_id;
  gchar *response;
  ClipmanHistoryItem *item;
  GList *_link;
  ClipmanHistory *history;
  gboolean decode_secure_text;

  g_variant_get (parameters, "(bq)", &decode_secure_text, &searched_id);

  history = clipman_history_get ();
  _link  = clipman_history_find_item_by_id(history, searched_id);

  if(_link == NULL)
    {
      g_dbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.Failed",
                                                  "item not found");
      return FALSE;
    }
  else
    {
      item = _link->data;
      if (decode_secure_text)
        {
          response = clipman_secure_text_decode(item->content.text);
          DBG("value: %s decoded utf-8: %f", item->content.text, response);
        }
      else
        {
          response = g_strdup_printf ("%s", item->content.text);
        }
      g_dbus_method_invocation_return_value (invocation,
                                             g_variant_new ("(s)", response));
      g_free (response);
    }

  return TRUE;
}

// delete item helpers

// used to store our user argument
typedef struct
{
  ClipmanCollector *collector;
  gchar *decoded_secure_text;
} ClipmanCallbackClearClipboard;

static void
clipman_dbus_callback_clipboard_request_text (
                         GtkClipboard *clipboard,
                         const gchar *text,
                         ClipmanCallbackClearClipboard *args)
{
  if (text == NULL || text[0] == '\0')
    return;

  if(g_strcmp0(text, args->decoded_secure_text) == 0)
  {
    clipman_collector_set_is_restoring(args->collector);
    gtk_clipboard_set_text (clipboard, "", 1);
  }
}

static gboolean
clipman_dbus_method_delete_item_by_id(
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation)
{
  guint16 searched_id;
  gboolean result;
  ClipmanHistory *history;
  GList *_link;

  g_variant_get (parameters, "(q)", &searched_id);

  history = clipman_history_get ();
  _link = clipman_history_find_item_by_id(history, searched_id);
  if (_link == NULL)
    {
      result = FALSE;
    }
  else
    {
      ClipmanHistoryItem *item;
      item = _link->data;

      if (item->type == CLIPMAN_HISTORY_TYPE_SECURE_TEXT)
        {
          // ensure clearing clipboard(s), if it was the same value
          ClipmanCallbackClearClipboard args;
          GtkClipboard *clipboard;

          args.collector = clipman_collector_get();
          args.decoded_secure_text = clipman_secure_text_decode(item->content.text);

          clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
          gtk_clipboard_request_text (
              clipboard,
              (GtkClipboardTextReceivedFunc)clipman_dbus_callback_clipboard_request_text,
              &args);

          // also check primary
          clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
          gtk_clipboard_request_text (
              clipboard,
              (GtkClipboardTextReceivedFunc)clipman_dbus_callback_clipboard_request_text,
              &args);
        }
      clipman_history_delete_item_by_pointer(history, _link);
      result = TRUE;
    }

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", result));
  return result;
}

static gboolean
clipman_dbus_method_add_item(
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation)
{
  const gchar *new_item_value;
  gboolean is_secure_item;
  ClipmanHistory *history;
  ClipmanHistoryId new_id;
  ClipmanCollector *collector;
  GList *new_item;

  g_variant_get (parameters, "(bs)", &is_secure_item, &new_item_value);

  history = clipman_history_get ();
  new_id = clipman_history_add_text (history, is_secure_item, new_item_value);

  // add newly created item to clipboard
  new_item = clipman_history_find_item_by_id(history, new_id);
  // prevent the new secure_text to be copied twice as disclosed item, because of encoding
  collector = clipman_collector_get();
  clipman_collector_set_is_restoring(collector);
  cb_set_clipboard(NULL, new_item->data);

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(q)", new_id));
  return TRUE;
}

static gboolean
clipman_dbus_method_clear_history(
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation)
{
  ClipmanHistory *history;
  gboolean clear_only_secure_text;
  guint nb_element_cleared;

  g_variant_get (parameters, "(b)", &clear_only_secure_text);
  history = clipman_history_get ();

  nb_element_cleared = clipman_history_clear(history, clear_only_secure_text);

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(q)", nb_element_cleared));
  return TRUE;
}

static gboolean
clipman_dbus_method_set_secure_by_id(
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation)
{
  ClipmanHistory *history;
  guint16 searched_id;
  gboolean secure, result;
  GList *_link;

  g_variant_get (parameters, "(bq)", &secure, &searched_id);
  history = clipman_history_get ();
  _link  = clipman_history_find_item_by_id(history, searched_id);

  result = FALSE;

  if(_link == NULL)
    {
      g_dbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.Failed",
                                                  "item not found");
      return FALSE;
    }
  else
    {
      ClipmanHistoryItem *item;

      item = _link->data;
      g_print("set_secure_by_id: item found: %d, %s\n", item->id, item->content.text);
      switch (item->type)
        {
        case CLIPMAN_HISTORY_TYPE_TEXT:
        case CLIPMAN_HISTORY_TYPE_SECURE_TEXT:
          result = clipman_history_change_secure_text_state(history, secure, item);
          break;
        default:
          // not applicable to non text item
          g_dbus_method_invocation_return_dbus_error (invocation,
                                                      "org.gtk.GDBus.Failed",
                                                      "item not a text item, cannont be secured");
        break;
        }
    }

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", result));

  return result;
}

static gboolean
clipman_dbus_method_collect_next_item_secure(
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation)
{
  guint16 nb_next_item_secured;
  ClipmanCollector *collector;

  g_variant_get (parameters, "(q)", &nb_next_item_secured);
  collector = clipman_collector_get();
  clipman_collector_set_nb_next_item_secured(collector, nb_next_item_secured);

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", TRUE));

  return TRUE;
}
// mapping table: map DBus method_name from xml to function pointer
ClipmanDbusMethod clipman_dbus_methods[] =
{
  { .name  =  "get_item_by_id",            .call  =  clipman_dbus_method_get_item_by_id           },
  { .name  =  "delete_item_by_id",         .call  =  clipman_dbus_method_delete_item_by_id        },
  { .name  =  "list_history",              .call  =  clipman_dbus_method_list_history             },
  { .name  =  "add_item",                  .call  =  clipman_dbus_method_add_item                 },
  { .name  =  "clear_history",             .call  =  clipman_dbus_method_clear_history            },
  { .name  =  "set_secure_by_id",          .call  =  clipman_dbus_method_set_secure_by_id         },
  { .name  =  "collect_next_item_secure",  .call  =  clipman_dbus_method_collect_next_item_secure },
  { .name  =  NULL,                        .call  =  NULL }
};

static void
clipman_dbus_handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  gboolean method_found;
  ClipmanDbusMethod *method;

  g_print("method_name: %s\n", method_name);

  method_found = FALSE;
  for (method = clipman_dbus_methods; method->name != NULL; method++)
  {
    if (g_strcmp0 (method_name, method->name) == 0)
      {
        method->call(parameters, invocation);
        method_found = TRUE;
        break;
      }
  }

  if (!method_found)
    {
      g_dbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.Failed",
                                                  "method_name not supported");
    }
}

static const GDBusInterfaceVTable clipman_dbus_interface_vtable =
{
  clipman_dbus_handle_method_call,
  clipman_dbus_handle_get_property,
  clipman_dbus_handle_set_property
};

static void
clipman_dbus_on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  guint registration_id;

  registration_id = g_dbus_connection_register_object (connection,
                                                       "/org/xfce/clipman/GDBus/service",
                                                       clipman_dbus_introspection_data->interfaces[0],
                                                       &clipman_dbus_interface_vtable,
                                                       NULL,  /* user_data */
                                                       NULL,  /* user_data_free_func */
                                                       NULL); /* GError** */
  g_assert (registration_id > 0);
}

static void
clipman_dbus_on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
}

static void
clipman_dbus_on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  exit (1);
}

guint
clipman_dbus_service_init(void)
{
  guint owner_id;
  // set out global variable
  clipman_dbus_introspection_data = g_dbus_node_info_new_for_xml (clipman_dbus_introspection_xml, NULL);
  if (clipman_dbus_introspection_data == NULL)
  {
    g_warning ("Error parsing the XML clipman_dbus_introspection_xml for DBus, DBus not started");
    return 0;
  }

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "org.xfce.clipman.GDBus.service",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             clipman_dbus_on_bus_acquired,
                             clipman_dbus_on_name_acquired,
                             clipman_dbus_on_name_lost,
                             NULL,
                             NULL);
  DBG("clipman GDBus initialized %d", owner_id);
  return owner_id;
}
