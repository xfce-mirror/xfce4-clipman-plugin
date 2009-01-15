/*
 *  Copyright (c) 2008-2009 Mike Massonnet <mmassonnet@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <xfconf/xfconf.h>
#include <glade/glade.h>

#include "common.h"
#include "settings-dialog_glade.h"
#include "collector.h"
#include "history.h"
#include "menu.h"



/*
 * MyPlugin structure
 */

typedef struct _MyPlugin MyPlugin;
struct _MyPlugin
{
  XfcePanelPlugin      *panel_plugin;
  XfconfChannel        *channel;
  ClipmanCollector     *collector;
  ClipmanHistory       *history;
  GtkWidget            *button;
  GtkWidget            *image;
  GtkWidget            *menu;
};

/*
 * Panel Plugin registration
 */

static void panel_plugin_register (XfcePanelPlugin *panel_plugin);
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (panel_plugin_register);

/*
 * Panel Plugin functions declarations
 */

static gboolean         panel_plugin_set_size           (XfcePanelPlugin *panel_plugin,
                                                         int size,
                                                         MyPlugin *plugin);
static void             panel_plugin_configure          (XfcePanelPlugin *panel_plugin,
                                                         MyPlugin *plugin);
static void             panel_plugin_load               (XfcePanelPlugin *panel_plugin,
                                                         MyPlugin *plugin);
static void             panel_plugin_save               (XfcePanelPlugin *panel_plugin,
                                                         MyPlugin *plugin);
static void             panel_plugin_free               (XfcePanelPlugin *panel_plugin,
                                                         MyPlugin *plugin);
static void             cb_button_toggled               (GtkToggleButton *button,
                                                         MyPlugin *plugin);
static void             cb_menu_deactivate              (GtkMenuShell *menu,
                                                         MyPlugin *plugin);
static void             my_plugin_position_menu         (GtkMenu *menu,
                                                         gint *x,
                                                         gint *y,
                                                         gboolean *push_in,
                                                         MyPlugin *plugin);



/*
 * Panel Plugin functions
 */

static void
panel_plugin_register (XfcePanelPlugin *panel_plugin)
{
  MyPlugin *plugin;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  xfconf_init (NULL);

  /* XfcePanelPlugin widget */
#if GTK_CHECK_VERSION (2,12,0)
  gtk_widget_set_tooltip_text (GTK_WIDGET (panel_plugin), _("Clipman"));
#endif

  /* MyPlugin */
  plugin = g_slice_new0 (MyPlugin);

  /* Keep a pointer on the panel plugin */
  plugin->panel_plugin = panel_plugin;

  /* XfconfChannel */
  plugin->channel = xfconf_channel_new_with_property_base ("xfce4-panel", "/plugins/clipman");

  /* ClipmanHistory */
  plugin->history = clipman_history_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/max-texts-in-history",
                          G_TYPE_UINT, plugin->history, "max-texts-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/max-images-in-history",
                          G_TYPE_UINT, plugin->history, "max-images-in-history");
  xfconf_g_property_bind (plugin->channel, "/settings/save-on-quit",
                          G_TYPE_BOOLEAN, plugin->history, "save-on-quit");

  /* ClipmanCollector */
  plugin->collector = clipman_collector_get ();
  xfconf_g_property_bind (plugin->channel, "/settings/add-primary-clipboard",
                          G_TYPE_BOOLEAN, plugin->collector, "add-primary-clipboard");

  /* Panel Button */
  plugin->button = xfce_create_panel_toggle_button ();
  /* The image is set through the set_size callback */
  plugin->image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->image);
  gtk_container_add (GTK_CONTAINER (panel_plugin), plugin->button);
  xfce_panel_plugin_add_action_widget (panel_plugin, plugin->button);
  g_signal_connect (plugin->button, "toggled",
                    G_CALLBACK (cb_button_toggled), plugin);

  /* ClipmanMenu */
  plugin->menu = clipman_menu_new ();
  g_signal_connect (plugin->menu, "deactivate",
                    G_CALLBACK (cb_menu_deactivate), plugin);

  /* Panel Plugin Signals */
  g_signal_connect (panel_plugin, "size-changed",
                    G_CALLBACK (panel_plugin_set_size), plugin);
  xfce_panel_plugin_menu_show_configure (panel_plugin);
  g_signal_connect (panel_plugin, "configure-plugin",
                    G_CALLBACK (panel_plugin_configure), plugin);
  g_signal_connect (panel_plugin, "save",
                    G_CALLBACK (panel_plugin_save), plugin);
  g_signal_connect (panel_plugin, "free-data",
                    G_CALLBACK (panel_plugin_free), plugin);

  /* Load the data */
  panel_plugin_load (panel_plugin, plugin);

  gtk_widget_show_all (GTK_WIDGET (panel_plugin));
}

static gboolean
panel_plugin_set_size (XfcePanelPlugin *panel_plugin,
                       int size,
                       MyPlugin *plugin)
{
  GdkPixbuf *pixbuf;

  gtk_widget_set_size_request (plugin->button, size, size);

  size -= 2 + 2 * MAX (plugin->button->style->xthickness,
                       plugin->button->style->ythickness);
  pixbuf = xfce_themed_icon_load (GTK_STOCK_PASTE, size);
  gtk_image_set_from_pixbuf (GTK_IMAGE (plugin->image), pixbuf);
  g_object_unref (G_OBJECT (pixbuf));

  return TRUE;
}

static void
panel_plugin_configure (XfcePanelPlugin *panel_plugin,
                        MyPlugin *plugin)
{
  GladeXML *gxml;
  GtkWidget *dialog;

  /* Get GladeXML and the dialog */
  gxml = glade_xml_new_from_buffer (settings_dialog_glade, settings_dialog_glade_length, NULL, NULL);
  dialog = glade_xml_get_widget (gxml, "settings-dialog");
  /* FIXME Doing this with Glade makes the panel button unclickable... but
   * still we need to destroy the dialog when the panel quits, so keep it. */
  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (panel_plugin))));

  /* Set default values */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "save-on-quit")),
                                DEFAULT_SAVE_ON_QUIT);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "add-selections")),
                                DEFAULT_ADD_PRIMARY_CLIPBOARD);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, "store-an-image")),
                                (gboolean)DEFAULT_MAX_IMAGES_IN_HISTORY);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (glade_xml_get_widget (gxml, "max-texts-in-history")),
                             (gdouble)DEFAULT_MAX_TEXTS_IN_HISTORY);

  /* Bind the Xfconf properties to the widgets */
  xfconf_g_property_bind (plugin->channel, "/settings/save-on-quit", G_TYPE_BOOLEAN,
                          G_OBJECT (glade_xml_get_widget (gxml, "save-on-quit")), "active");
  xfconf_g_property_bind (plugin->channel, "/settings/add-primary-clipboard", G_TYPE_BOOLEAN,
                          G_OBJECT (glade_xml_get_widget (gxml, "add-selections")), "active");
  xfconf_g_property_bind (plugin->channel, "/settings/max-images-in-history", G_TYPE_UINT,
                          G_OBJECT (glade_xml_get_widget (gxml, "store-an-image")), "active");
  xfconf_g_property_bind (plugin->channel, "/settings/max-texts-in-history", G_TYPE_UINT,
                          G_OBJECT (glade_xml_get_widget (gxml, "max-texts-in-history")), "value");

  /* Run the dialog */
  xfce_panel_plugin_block_menu (panel_plugin);
  gtk_dialog_run (GTK_DIALOG (dialog));
  xfce_panel_plugin_unblock_menu (panel_plugin);

  gtk_widget_destroy (dialog);
  g_object_unref (gxml);
}

static void
panel_plugin_load (XfcePanelPlugin *panel_plugin,
                   MyPlugin *plugin)
{
  GtkClipboard *clipboard;
  GKeyFile *keyfile;
  gchar **texts = NULL;
  gchar *filename;
  GdkPixbuf *image;
  gint i = 0;
  gboolean save_on_quit;

  /* Return if the history must not be saved */
  g_object_get (plugin->history, "save-on-quit", &save_on_quit, NULL);
  if (save_on_quit == FALSE)
    return;

  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

  /* Load images */
  while (TRUE)
    {
      filename = g_strdup_printf ("%s/xfce4/clipman/image%d.png", g_get_user_cache_dir (), i++);
      DBG ("Loading image from cache file %s", filename);
      image = gdk_pixbuf_new_from_file (filename, NULL);
      g_unlink (filename);
      g_free (filename);
      if (image == NULL)
        break;

      clipman_history_add_image (plugin->history, image, clipboard);
      g_object_unref (image);
    }

  /* Load texts */
  filename = g_strdup_printf ("%s/xfce4/clipman/textsrc", g_get_user_cache_dir ());
  DBG ("Loading texts from cache file %s", filename);
  keyfile = g_key_file_new ();
  if (g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, NULL))
    {
      texts = g_key_file_get_string_list (keyfile, "texts", "texts", NULL, NULL);
      for (i = 0; texts != NULL && texts[i] != NULL; i++)
        clipman_history_add_text (plugin->history, texts[i], clipboard);
      g_unlink (filename);
    }

  g_key_file_free (keyfile);
  g_strfreev (texts);
  g_free (filename);

  /* Set no current item */
  clipman_history_set_item_to_restore (plugin->history, NULL);
}

static void
panel_plugin_save (XfcePanelPlugin *panel_plugin,
                   MyPlugin *plugin)
{
  GSList *list, *l;
  const ClipmanHistoryItem *item;
  GKeyFile *keyfile;
  const gchar **texts;
  gchar *data;
  gchar *filename;
  gint n_texts, n_images;
  gboolean save_on_quit;

  /* Return if the history must not be saved */
  g_object_get (plugin->history, "save-on-quit", &save_on_quit, NULL);
  if (save_on_quit == FALSE)
    return;

  /* Create initial directory */
  filename = xfce_resource_save_location (XFCE_RESOURCE_CACHE, "xfce4/clipman/", TRUE);
  g_free (filename);

  /* Save the history */
  list = clipman_history_get_list (plugin->history);
  if (list != NULL)
    {
      texts = g_malloc0 (g_slist_length (list) * sizeof (gchar *));
      for (n_texts = n_images = 0, l = list; l != NULL; l = l->next)
        {
          item = l->data;

          switch (item->type)
            {
            case CLIPMAN_HISTORY_TYPE_TEXT:
              texts[n_texts++] = item->content.text;
              break;

            case CLIPMAN_HISTORY_TYPE_IMAGE:
              filename = g_strdup_printf ("%s/xfce4/clipman/image%d.png", g_get_user_cache_dir (), n_images++);
              if (!gdk_pixbuf_save (item->content.image, filename, "png", NULL, NULL))
                g_debug ("Failed to save image to cache file %s", filename);
              else
                DBG ("Saved image to cache file %s", filename);
              g_free (filename);
              break;

            default:
              g_assert_not_reached ();
            }
        }

      if (n_texts > 0)
        {
          filename = g_strdup_printf ("%s/xfce4/clipman/textsrc", g_get_user_cache_dir ());
          keyfile = g_key_file_new ();
          g_key_file_set_string_list (keyfile, "texts", "texts", texts, n_texts);
          data = g_key_file_to_data (keyfile, NULL, NULL);
          g_file_set_contents (filename, data, -1, NULL);
          DBG ("Saved texts to cache file %s", filename);

          g_key_file_free (keyfile);
          g_free (data);
          g_free (filename);
          g_free (texts);
        }

      g_slist_free (list);
    }
}

static void
panel_plugin_free (XfcePanelPlugin *panel_plugin,
                   MyPlugin *plugin)
{
  gtk_widget_destroy (plugin->menu);
  gtk_widget_destroy (plugin->button);
  g_object_unref (plugin->channel);
  g_object_unref (plugin->collector);
  g_object_unref (plugin->history);
  g_slice_free (MyPlugin, plugin);
  xfconf_shutdown ();
}

static void
cb_button_toggled (GtkToggleButton *button,
                   MyPlugin *plugin)
{
  if (gtk_toggle_button_get_active (button))
    {
      gtk_menu_set_screen (GTK_MENU (plugin->menu), gtk_widget_get_screen (plugin->button));
      xfce_panel_plugin_register_menu (plugin->panel_plugin, GTK_MENU (plugin->menu));
      gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                      (GtkMenuPositionFunc)my_plugin_position_menu, plugin,
                      0, gtk_get_current_event_time ());
    }
}

static void
cb_menu_deactivate (GtkMenuShell *menu,
                    MyPlugin *plugin)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), FALSE);
}

static void
my_plugin_position_menu (GtkMenu *menu,
                         gint *x,
                         gint *y,
                         gboolean *push_in,
                         MyPlugin *plugin)
{
  GtkWidget *button;
  GtkRequisition requisition;
  GtkOrientation orientation;

  button = plugin->button;
  orientation = xfce_panel_plugin_get_orientation (plugin->panel_plugin);
  gtk_widget_size_request (GTK_WIDGET (menu), &requisition);
  gdk_window_get_origin (button->window, x, y);

  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      if (*y + button->allocation.height + requisition.height > gdk_screen_height ())
        /* Show menu above */
        *y -= requisition.height;
      else
        /* Show menu below */
        *y += button->allocation.height;

      if (*x + requisition.width > gdk_screen_width ())
        /* Adjust horizontal position */
        *x = gdk_screen_width () - requisition.width;
      break;

    case GTK_ORIENTATION_VERTICAL:
      if (*x + button->allocation.width + requisition.width > gdk_screen_width ())
        /* Show menu on the right */
        *x -= requisition.width;
      else
        /* Show menu on the left */
        *x += button->allocation.width;

      if (*y + requisition.height > gdk_screen_height ())
        /* Adjust vertical position */
        *y = gdk_screen_height () - requisition.height;
      break;

    default:
      break;
    }
}

