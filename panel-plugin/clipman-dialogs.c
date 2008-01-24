/*  $Id: clipman-dialogs.c 2395 2007-01-17 17:42:53Z nick $
 *
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "clipman.h"
#include "clipman-dialogs.h"



static void                     clipman_configure_response          (GtkWidget *dialog,
                                                                     gint response,
                                                                     ClipmanOptions *options);
static void                     set_scale_to_spin                   (GtkWidget *scalewidget,
                                                                     GtkWidget *spinwidget);
static void                     set_spin_to_scale                   (GtkWidget *spinwidget,
                                                                     GtkWidget *scalewidget);
static void                     toggle_button                       (GtkWidget *button,
                                                                     ClipmanOptions *options);



void
clipman_configure_new (ClipmanPlugin *clipman)
{
  GtkWidget      *dialog, *dialog_vbox, *frame, *button, *label;
  GtkWidget      *vbox, *hbox, *notebook_vbox, *notebook;
  ClipmanOptions *options;
  GSList         *group;

  options = panel_slice_new0 (ClipmanOptions);
  options->clipman = clipman;

  xfce_panel_plugin_block_menu (clipman->panel_plugin);

  dialog = xfce_titled_dialog_new_with_buttons (_("Clipboard Manager"),
												GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (clipman->panel_plugin))),
												GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
												GTK_STOCK_HELP, GTK_RESPONSE_HELP,
												GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
												NULL);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

  g_object_set_data (G_OBJECT (clipman->panel_plugin), "dialog", dialog);

  dialog_vbox = GTK_DIALOG (dialog)->vbox;

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (dialog_vbox), notebook, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), BORDER-3);

  notebook_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (notebook), notebook_vbox);

  /**
   * The general frame
   **/
  vbox = gtk_vbox_new (FALSE, 2);

  frame = xfce_create_framebox_with_content (_("Configuration"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  button = options->ExitSave = gtk_check_button_new_with_mnemonic (_("Save clipboard contents on _exit"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->clipman_clips->save_on_exit);

  g_signal_connect (G_OBJECT (button), "toggled",
					G_CALLBACK (toggle_button), options);

  button = options->IgnoreSelection = gtk_check_button_new_with_mnemonic (_("_Ignore selections"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->clipman_clips->ignore_primary);

  g_signal_connect (G_OBJECT (button), "toggled",
					G_CALLBACK (toggle_button), options);

  button = options->PreventEmpty = gtk_check_button_new_with_mnemonic (_("Pre_vent empty clipboard"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->clipman_clips->prevent_empty);

  g_signal_connect (G_OBJECT (button), "toggled",
					G_CALLBACK (toggle_button), options);

  label = gtk_label_new (_("<b>General</b>"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_padding (GTK_MISC (label), 2, 0);

  /**
   * separate clipboards frame
   **/
  vbox = gtk_vbox_new (FALSE, 2);

  frame = xfce_create_framebox_with_content (_("Configuration"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  group = NULL;

  button = options->Behaviour = gtk_radio_button_new_with_mnemonic (group, _("Normal clipboard _management"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

  if(clipman->clipman_clips->behavior == NORMAL)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  button = gtk_radio_button_new_with_mnemonic (group, _("Strictly separate _both clipboards"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

  if(clipman->clipman_clips->behavior == STRICTLY)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  label = gtk_label_new (_("<b>Clipboard Behaviour</b>"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_padding (GTK_MISC (label), 2, 0);

  /**
   * Notebook label
   **/
  label = gtk_label_new (_("General"));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0), label);

  notebook_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (notebook), notebook_vbox);

  /**
   * Menu appearance frame
   **/
  vbox = gtk_vbox_new (FALSE, 2);

  frame = xfce_create_framebox_with_content (_("Configuration"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  button = options->ItemNumbers = gtk_check_button_new_with_mnemonic (_("_Show item numbers"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->menu_item_show_number);

  g_signal_connect (G_OBJECT (button), "toggled",
					G_CALLBACK (toggle_button), options);

  label = gtk_label_new (_("<b>Menu Appearance</b>"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_padding (GTK_MISC (label), 2, 0);

  /**
   * Call some functions
   **/

  toggle_button (options->IgnoreSelection, options);

  /**
   * Numbers frame
   **/
  vbox = gtk_vbox_new (FALSE, 2);

  frame = xfce_create_framebox_with_content (_("Configuration"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  label = gtk_label_new (_("Clipboard history items:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  button = options->HistorySize = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (clipman->clipman_clips->history_length, MINHISTORY, MAXHISTORY, 1, 5, 0)));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 10);
  gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
  gtk_scale_set_digits (GTK_SCALE (button), 0);

  button = gtk_spin_button_new_with_range(MINHISTORY, MAXHISTORY, 1);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), clipman->clipman_clips->history_length);

  /* Sync history widgets */
  g_signal_connect (G_OBJECT (options->HistorySize), "value_changed",
					G_CALLBACK (set_scale_to_spin), button);

  g_signal_connect (G_OBJECT (button), "value_changed",
					G_CALLBACK (set_spin_to_scale), options->HistorySize);

  label = gtk_label_new (_("Menu item characters:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  button = options->ItemChars = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (clipman->menu_item_max_chars, MINCHARS, MAXCHARS, 1, 5, 0)));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 10);
  gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
  gtk_scale_set_digits (GTK_SCALE (button), 0);

  button = gtk_spin_button_new_with_range(MINCHARS, MAXCHARS, 1);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), clipman->menu_item_max_chars);

  g_signal_connect (G_OBJECT (options->ItemChars), "value_changed",
					G_CALLBACK (set_scale_to_spin), button);

  g_signal_connect (G_OBJECT (button), "value_changed",
					G_CALLBACK (set_spin_to_scale), options->ItemChars);

  label = gtk_label_new (_("<b>Numbers</b>"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), label);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_padding (GTK_MISC (label), 4, 0);

  label = gtk_label_new (_("Appearance"));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1), label);

  g_signal_connect (dialog, "response",
				   G_CALLBACK (clipman_configure_response), options);

  gtk_widget_show_all (dialog);
}

static void
clipman_configure_response (GtkWidget *dialog,
                            gint response,
                            ClipmanOptions *options)
{
  gboolean result;

  if (response == GTK_RESPONSE_HELP)
	{
	  /* show help */
	  result = g_spawn_command_line_async ("exo-open --launch WebBrowser " PLUGIN_WEBSITE, NULL);

	  if (G_UNLIKELY (result == FALSE))
		g_warning (_("Unable to open the following url: %s"), PLUGIN_WEBSITE);

	  return;
	}

  DBG("Destroy the dialog");

  g_object_set_data (G_OBJECT (options->clipman->panel_plugin), "dialog", NULL);

  /* Behavior */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->Behaviour)))
	options->clipman->clipman_clips->behavior = NORMAL;
  else
	options->clipman->clipman_clips->behavior = STRICTLY;

  /* History length */
  gint history_length = gtk_range_get_value (GTK_RANGE (options->HistorySize));
  if (options->clipman->clipman_clips->history_length != history_length)
    {
      options->clipman->clipman_clips->history_length = history_length;

      /* Free the overlap in the history */
      ClipmanClip *clip;
      gint length = g_slist_length (options->clipman->clipman_clips->history);
      while (history_length < length--)
        {
          clip = (ClipmanClip *)(g_slist_last (options->clipman->clipman_clips->history)->data);
          options->clipman->clipman_clips->history =
            g_slist_remove (options->clipman->clipman_clips->history, clip);

          g_free (clip->text);
          g_free (clip->short_text);
          g_slice_free (ClipmanClip, clip);
        }
    }

  /* Menu item max chars */
  gint max_chars = gtk_range_get_value (GTK_RANGE (options->HistorySize));
  if (options->clipman->menu_item_max_chars != max_chars)
    {
      options->clipman->menu_item_max_chars = gtk_range_get_value (GTK_RANGE (options->ItemChars));

      /* Invalidate the old clip->short_text */
      gint i = 0;
      ClipmanClip *clip;
      while (NULL != (clip = (ClipmanClip *)g_slist_nth_data (options->clipman->clipman_clips->history, i++)))
        {
          g_free (clip->short_text);
          clip->short_text = NULL;
        }
    }

  xfce_panel_plugin_unblock_menu (options->clipman->panel_plugin);

  gtk_widget_destroy (dialog);

  panel_slice_free (ClipmanOptions, options);
}

static void
set_scale_to_spin (GtkWidget *scalewidget,
                   GtkWidget *spinwidget)
{
  guint value;
  value = gtk_range_get_value (GTK_RANGE (scalewidget));
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinwidget), value);
}

static void
set_spin_to_scale (GtkWidget *spinwidget,
                   GtkWidget *scalewidget)
{
  guint value;
  value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinwidget));
  gtk_range_set_value (GTK_RANGE (scalewidget), value);
}

static void
toggle_button (GtkWidget *button,
               ClipmanOptions *options)
{
  if (button == options->ExitSave)
	options->clipman->clipman_clips->save_on_exit =
	  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == options->IgnoreSelection)
	options->clipman->clipman_clips->ignore_primary =
	  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == options->PreventEmpty)
	options->clipman->clipman_clips->prevent_empty =
	  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == options->ItemNumbers)
	options->clipman->menu_item_show_number =
	  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
}

