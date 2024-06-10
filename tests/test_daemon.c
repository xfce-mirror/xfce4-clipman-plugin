#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clipboard-manager/clipboard-manager.h"

#include <glib.h>
#include <gtk/gtk.h>

int
main (int argc,
      char *argv[])
{
  XcpClipboardManager *daemon;

  gtk_init (&argc, &argv);

  daemon = xcp_clipboard_manager_get ();

  gtk_main ();

  g_object_unref (daemon);

  return 0;
}
