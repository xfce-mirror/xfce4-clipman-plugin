#include <gtk/gtk.h>
#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>

#include "history.h"
#include "dbus-clipman-service.h"

static GDBusNodeInfo *clipman_dbus_introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const gchar clipman_dbus_introspection_xml[] =
  "<node>"
  "  <interface name='org.xfce.clipman.GDBus.service'>"
  "    <annotation name='org.gtk.GDBus.Annotation' value='OnInterface'/>"
  "    <annotation name='org.gtk.GDBus.Annotation' value='AlsoOnInterface'/>"
  "    <method name='list_history'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='history_list_as_string' direction='out'/>"
  "    </method>"
  "    <method name='get_item_by_id'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='OnMethod'/>"
  "      <arg type='u' name='searched_id' direction='in'/>"
  "      <arg type='s' name='text_item_value' direction='out'/>"
  "    </method>"
  "    <method name='delete_item_by_id'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='OnMethod'/>"
  "      <arg type='u' name='item_id' direction='in'/>"
  "      <arg type='b' name='result' direction='out'/>"
  "    </method>"
  "    <method name='add_item'>"
  "      <annotation name='org.gtk.GDBus.Annotation' value='OnMethod'/>"
  "      <arg type='b' name='secure' direction='in'/>"
  "      <arg type='s' name='value' direction='in'/>"
  "      <arg type='q' name='new_id' direction='out'/>"
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
  GSList *list, *l;
  ClipmanHistoryItem *item;

  history = clipman_history_get ();

  list = clipman_history_get_list (history);
  list = g_slist_reverse (list);
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
      gchar *text;

      for (l = list; l != NULL; l = l->next)
        {
          item = l->data;

          switch (item->type)
            {
            /* We ignore everything but text (no images or QR codes) */
            case CLIPMAN_HISTORY_TYPE_TEXT:
              // we currently handle new lines in clipboard content as ==> \n
              // which is not revertable change, but enough for the poc.
              split  = g_strsplit(item->content.text, "\n", -1);
              text   = g_strjoinv("\\n", split);
              g_strfreev(split);
              if(response == NULL)
                {
                  response = g_strdup_printf ("%d %s", item->id, text);
                }
              else
                {
                  tmp = g_strdup_printf ("%s\n%d %s", response, item->id, text);
                  g_free (response);
                  response = tmp;
                }
              g_free(text);
              break;
            case CLIPMAN_HISTORY_TYPE_SECURE_TEXT:
              if(response == NULL)
                {
                  response = g_strdup_printf ("%d %s", item->id, item->preview.text);
                }
              else
                {
                  tmp = g_strdup_printf ("%s\n%d %s", response, item->id, item->preview.text);
                  g_free (response);
                  response = tmp;
                }
              break;

            default:
              DBG("Ignoring non-text history type %d", item->type);
              continue;
            }
        }

      g_slist_free (list);
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
  ClipmanHistory *history;


  g_variant_get (parameters, "(u)", &searched_id);
  history = clipman_history_get ();
  item = clipman_history_find_item_by_id(history, searched_id);

  if(item == NULL)
    {
      g_dbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.Failed",
                                                  "item not found");
      return FALSE;
    }
  else
    {
      response = g_strdup_printf ("%s", item->content.text);
      g_dbus_method_invocation_return_value (invocation,
                                             g_variant_new ("(s)", response));
      g_free (response);
    }

  return TRUE;
}

static gboolean
clipman_dbus_method_delete_item_by_id(
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation)
{
  guint searched_id;
  gboolean result;
  ClipmanHistory *history;

  g_variant_get (parameters, "(u)", &searched_id);

  history = clipman_history_get ();
  result = clipman_history_delete_item_by_id(history, searched_id);

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

  g_variant_get (parameters, "(bs)", &is_secure_item, &new_item_value);

  history = clipman_history_get ();
  new_id = clipman_history_add_text (history, is_secure_item, new_item_value);

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(q)", new_id));
  return TRUE;
}

// mappping table: map DBus method_name from xml to function pointer
ClipmanDbusMethod clipman_dbus_methods[] =
{
  { .name  =  "get_item_by_id",     .call  =  clipman_dbus_method_get_item_by_id     },
  { .name  =  "delete_item_by_id",  .call  =  clipman_dbus_method_delete_item_by_id  },
  { .name  =  "list_history",       .call  =  clipman_dbus_method_list_history       },
  { .name  =  "add_item",           .call  =  clipman_dbus_method_add_item           },
  { .name  =  NULL,                 .call  =  NULL                                   }
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
