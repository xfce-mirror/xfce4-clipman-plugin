// copied from https://developer.gnome.org/gtk3/stable/gtk-getting-started.html
#include <gtk/gtk.h>

#include "history.h"


int
main (int    argc,
      char **argv)
{
  //GtkApplication *app;
  int status = 0;
  GSList *list, *l;
	ClipmanHistory *history;
  ClipmanHistoryItem *item;
  int i;

  //app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  //g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  //status = g_application_run (G_APPLICATION (app), argc, argv);
  //g_object_unref (app);

  if (!clipman_history_clipman_daemon_running ())
    {
      printf("The clipboard daemon is not running, exiting. You can launch it with 'xfce4-clipman'.\n");
      return FALSE;
    }

	history = clipman_history_get ();
  list = clipman_history_get_list (history);

  if (list == NULL)
    {
      printf("Clipboard is empty\n");
    }
  else
    {
      for (l = list, i = 0; l != NULL; l = l->next, i++)
        {
          item = l->data;

          switch (item->type)
            {
            /* We ignore everything but text (no images or QR codes) */
            case CLIPMAN_HISTORY_TYPE_TEXT:
	            printf("%d %s\n", i, item->content.text);
//              gtk_list_store_insert_with_values (liststore, &iter, i,
//                                                 COLUMN_PREVIEW, item->preview.text,
//                                                 COLUMN_TEXT, item->content.text,
//                                                 -1);
              break;

            default:
              printf("Ignoring non-text history type %d", item->type);
              continue;
            }
        }

      g_slist_free (list);
    }

  return status;
}
