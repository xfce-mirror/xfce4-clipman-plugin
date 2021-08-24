#ifndef __CLIPMAN_DBUS_CLIPMAN_SERVICE_H
#define __CLIPMAN_DBUS_CLIPMAN_SERVICE_H

#include <gtk/gtk.h>

typedef
gboolean (*ClipmanDbusMethodPointer) (GVariant  *parameters, GDBusMethodInvocation *invocation);

typedef struct
{
  gchar                     *name;
  ClipmanDbusMethodPointer   call;
} ClipmanDbusMethod;

guint
clipman_dbus_service_init(void)
;

#endif /* !__CLIPMAN_DBUS_CLIPMAN_SERVICE_H */
