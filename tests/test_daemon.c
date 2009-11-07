#include <glib.h>
#include <gtk/gtk.h>
#include <daemon/daemon.h>

int main (int argc, char *argv[])
{
  GsdClipboardManager *daemon;

  gtk_init (&argc, &argv);

  daemon = gsd_clipboard_manager_new ();
  gsd_clipboard_manager_start (daemon, NULL);

  gtk_main ();

  return 0;
}

