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

  g_print("method_name: %s", method_name);

  if (g_strcmp0 (method_name, "list_history") == 0)
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
          g_dbus_method_invocation_return_value (invocation, NULL);
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
                  // we currently andle new lines in clipboard content as ==> \n
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

                default:
                  DBG("Ignoring non-text history type %d", item->type);
                  continue;
                }
            }

          g_slist_free (list);
          g_dbus_method_invocation_return_value (invocation,
                                                 g_variant_new ("(s)", response));
          g_free (response);
        }
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
