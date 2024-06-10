#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>

gboolean print_text = FALSE;

static void
cb (GtkClipboard *clipboard,
    GdkEvent *event,
    gpointer udata)
{
  GtkSelectionData *selection_data;
  GdkAtom *atoms;
  gchar *atom_name;
  gint n_atoms;

  if (!gtk_clipboard_wait_for_targets (clipboard, &atoms, &n_atoms))
    return;

  for (gint i = 0; i < n_atoms; i++)
    {
      if (atoms[i] == gdk_atom_intern_static_string ("TARGETS")
          || atoms[i] == gdk_atom_intern_static_string ("MULTIPLE")
          || atoms[i] == gdk_atom_intern_static_string ("DELETE")
          || atoms[i] == gdk_atom_intern_static_string ("INSERT_PROPERTY")
          || atoms[i] == gdk_atom_intern_static_string ("INSERT_SELECTION")
          || atoms[i] == gdk_atom_intern_static_string ("PIXMAP"))
        continue;

      atom_name = gdk_atom_name (atoms[i]);
      selection_data = gtk_clipboard_wait_for_contents (clipboard, atoms[i]);
      if (selection_data != NULL)
        {
          g_print ("%s (f:%d, l:%d)\n", atom_name, gtk_selection_data_get_format (selection_data), gtk_selection_data_get_length (selection_data));
          gtk_selection_data_free (selection_data);
        }
      else
        {
          g_print ("%s\n", atom_name);
        }
      g_free (atom_name);
    }
  g_print ("\n");

  g_free (atoms);

  if (print_text)
    {
      gchar *text = gtk_clipboard_wait_for_text (clipboard);
      g_print ("text: %.30s[...]\n\n", text);
      g_free (text);
    }
}

int
main (int argc,
      char *argv[])
{
  GtkClipboard *clipboard;

  gtk_init (&argc, &argv);

  if (argc > 1 && !g_strcmp0 (argv[1], "-p"))
    print_text = TRUE;
  else if (argc > 2 && !g_strcmp0 (argv[2], "-p"))
    print_text = TRUE;

  if (argc > 1 && !g_strcmp0 (argv[1], "-s"))
    clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  else
    clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  g_signal_connect (clipboard, "owner-change", G_CALLBACK (cb), NULL);

  gtk_main ();

  return 0;
}
