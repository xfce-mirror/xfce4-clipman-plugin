#include <glib.h>
#include <gtk/gtk.h>

static void
cb (GtkClipboard *clipboard,
    GdkEvent *event,
    gpointer udata)
{
  GdkAtom *atoms;
  gint n_atoms;

  if (!gtk_clipboard_wait_for_targets (clipboard, &atoms, &n_atoms))
    return;

  gint i;
  for (i = 0; i < n_atoms; i++)
    {
      gchar *name = gdk_atom_name (atoms[i]);
      g_print ("%s\n", name);
      g_free (name);
    }
  g_print ("\n");

  g_free (atoms);
}

int main (int argc, char *argv[])
{
  GtkClipboard *clipboard;

  gtk_init (&argc, &argv);

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  g_signal_connect (clipboard, "owner-change", G_CALLBACK (cb), NULL);

  gtk_main ();

  return 0;
}

