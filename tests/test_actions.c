#include <glib.h>
#include <gtk/gtk.h>
#include <panel-plugin/actions.h>

int main (int argc, char *argv[])
{
  ClipmanActions *actions;

  gtk_init (&argc, &argv);

  actions = clipman_actions_get ();
  clipman_actions_add (actions, "Image", ".*\\.(gif|jpg|png)", "GPicView", "gpicview \\0");
  clipman_actions_add (actions, "Image", NULL, "Gimp", "gimp \\0");
  clipman_actions_add (actions, "Image", NULL, "Gimp (remote)", "gimp-remote \\0");
  clipman_actions_add (actions, "Image", NULL, "GPicView", "gpicview2 \\0");
  clipman_actions_add (actions, "Print", ".*\\.png", "Print with GTK+", "gtk-print \\0");
  clipman_actions_match_with_menu (actions, "/imagE.png");
  clipman_actions_add (actions, "Text", "(txt)", "Mousepad", "mousepad \\0");

  clipman_actions_save (actions);

  gtk_main ();

  clipman_actions_remove_command (actions, "Image", "GPicView");
  clipman_actions_remove_command (actions, "Image", "GPicView");
  clipman_actions_remove_command (actions, "Image", "view");
  clipman_actions_remove_command (actions, "Image", "Ristretto");
  clipman_actions_remove_command (actions, "Text", "Mousepad");

  return 0;
}

