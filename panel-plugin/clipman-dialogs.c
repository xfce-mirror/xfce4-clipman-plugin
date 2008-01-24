/*  $Id: clipman-dialogs.c 2395 2007-01-17 17:42:53Z nick $
 *
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *  Copyright (c)      2007 Mike Massonnet <mmassonnet@xfce.org>
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



#define PLUGIN_WEBSITE "http://goodies.xfce.org/projects/panel-plugins/xfce4-clipman-plugin"

typedef struct _ClipmanOptions      ClipmanOptions;

struct _ClipmanOptions
{
  ClipmanPlugin        *clipman;

  GtkWidget            *ExitSave;
  GtkWidget            *PreventEmpty;
  GtkWidget            *IgnoreSelection;

  GtkWidget            *BehaviourNormal;
  GtkWidget            *BehaviourStrictly;

  GtkWidget            *ItemNumbers;

  GtkWidget            *HistorySize;
  GtkWidget            *ItemChars;

  GtkWidget            *IgnoreStatic;
  GtkWidget            *StaticDefault;
  GtkWidget            *StaticPrimary;
  GtkWidget            *StaticBoth;

};



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
clipman_configure_new (ClipmanPlugin *clipman_plugin)
{
  GtkWidget            *dialog, *dialog_vbox, *frame, *notebook, *notebook_vbox;
  GtkWidget            *vbox, *hbox, *button, *label;
  ClipmanClips         *clipman_clips = clipman_plugin->clipman_clips;
  ClipmanOptions       *options;
  GSList               *group;

  options = panel_slice_new0 (ClipmanOptions);
  options->clipman = clipman_plugin;

  dialog =
    xfce_titled_dialog_new_with_buttons (_("Clipboard Manager"),
                                         GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (clipman_plugin->panel_plugin))),
                                         GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                         GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                         GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                         NULL);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PASTE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
  gtk_window_stick (GTK_WINDOW (dialog));

  dialog_vbox = GTK_DIALOG (dialog)->vbox;

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (dialog_vbox), notebook, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), BORDER-2);

  /* === General === */
  notebook_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (notebook), notebook_vbox);

  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (notebook),
                                   gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0),
                                   _("General"));

  /* General */
  vbox = gtk_vbox_new (FALSE, 2);
  frame = xfce_create_framebox_with_content (_("General"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  button = options->ExitSave =
    gtk_check_button_new_with_mnemonic (_("Save clipboard contents on _exit"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->save_on_exit);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  button = options->PreventEmpty =
    gtk_check_button_new_with_mnemonic (_("Pre_vent empty clipboard"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->prevent_empty);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  button = options->IgnoreSelection =
    gtk_check_button_new_with_mnemonic (_("_Ignore selections"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->ignore_primary);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  /* Behavior */
  vbox = gtk_vbox_new (FALSE, 2);
  frame = xfce_create_framebox_with_content (_("Clipboard Behavior"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  group = NULL;

  button = options->BehaviourNormal =
    gtk_radio_button_new_with_mnemonic (group, _("Normal clipboard _management"));
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->behavior == NORMAL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  button = options->BehaviourStrictly =
    gtk_radio_button_new_with_mnemonic (group, _("Strictly separate _both clipboards"));
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->behavior == STRICTLY);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  /* === Appearance === */
  notebook_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (notebook), notebook_vbox);

  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (notebook),
                                   gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1),
                                   _("Appearance"));

  /* Appearance */
  vbox = gtk_vbox_new (FALSE, 2);
  frame = xfce_create_framebox_with_content (_("Menu Appearance"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  button = options->ItemNumbers = gtk_check_button_new_with_mnemonic (_("_Show item numbers"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_plugin->menu_item_show_number);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  /* Numbers */
  vbox = gtk_vbox_new (FALSE, 2);
  frame = xfce_create_framebox_with_content (_("Numbers"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  /* History length */
  label = gtk_label_new (_("Clipboard history items:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  button = options->HistorySize =
    gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (clipman_clips->history_length,
                                                        MINHISTORY, MAXHISTORY, 1, 5, 0)));
  gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
  gtk_scale_set_digits (GTK_SCALE (button), 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 10);

  button = gtk_spin_button_new_with_range (MINHISTORY, MAXHISTORY, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), clipman_clips->history_length);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  g_signal_connect (G_OBJECT (options->HistorySize), "value_changed",
                    G_CALLBACK (set_scale_to_spin), button);
  g_signal_connect (G_OBJECT (button), "value_changed",
                    G_CALLBACK (set_spin_to_scale), options->HistorySize);

  /* Max menu item chars */
  label = gtk_label_new (_("Menu item characters:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  button = options->ItemChars =
    gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (clipman_plugin->menu_item_max_chars,
                                                        MINCHARS, MAXCHARS, 1, 5, 0)));
  gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
  gtk_scale_set_digits (GTK_SCALE (button), 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 10);

  button = gtk_spin_button_new_with_range (MINCHARS, MAXCHARS, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button), clipman_plugin->menu_item_max_chars);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  g_signal_connect (G_OBJECT (options->ItemChars), "value_changed",
                    G_CALLBACK (set_scale_to_spin), button);
  g_signal_connect (G_OBJECT (button), "value_changed",
                    G_CALLBACK (set_spin_to_scale), options->ItemChars);


  /* === Static clipboard === */
  notebook_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (notebook), notebook_vbox);

  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (notebook),
                                   gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 2),
                                   _("Static Clipboard"));

  /* General */
  vbox = gtk_vbox_new (FALSE, 2);
  frame = xfce_create_framebox_with_content (_("General"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  button = options->IgnoreStatic =
    gtk_check_button_new_with_mnemonic (_("Ignore _static clipboard"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->ignore_static_clipboard);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  /* Copy to clipboard */
  vbox = gtk_vbox_new (FALSE, 2);
  frame = xfce_create_framebox_with_content (_("Select to Clipboards"), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);

  group = NULL;

  button = options->StaticDefault =
    gtk_radio_button_new_with_mnemonic (group, _("Default"));
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->static_selection == DEFAULT);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  button = options->StaticPrimary =
    gtk_radio_button_new_with_mnemonic (group, _("Selection"));
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->static_selection == PRIMARY);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  button = options->StaticBoth =
    gtk_radio_button_new_with_mnemonic (group, _("Both"));
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman_clips->static_selection == BOTH);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (toggle_button), options);

  /* Show the stuff */
  g_signal_connect (dialog, "response",
                    G_CALLBACK (clipman_configure_response), options);
  xfce_panel_plugin_block_menu (clipman_plugin->panel_plugin);
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
          clipman_clips_delete (options->clipman->clipman_clips, clip);
        }
    }

  /* Menu item max chars */
  gint max_chars = gtk_range_get_value (GTK_RANGE (options->HistorySize));
  if (options->clipman->menu_item_max_chars != max_chars)
    {
      options->clipman->menu_item_max_chars = gtk_range_get_value (GTK_RANGE (options->ItemChars));

      /* Invalidate the old clip->short_text */
      GSList *list;
      ClipmanClip *clip;
      for (list = options->clipman->clipman_clips->history; list != NULL; list = list->next)
        {
          clip = (ClipmanClip *)list->data;
          g_free (clip->short_text);
          clip->short_text = NULL;
        }
      for (list = options->clipman->clipman_clips->static_clipboard; list != NULL; list = list->next)
        {
          clip = (ClipmanClip *)list->data;
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
  ClipmanPlugin        *clipman_plugin = options->clipman;
  ClipmanClips         *clipman_clips = clipman_plugin->clipman_clips;

  if (button == options->ItemNumbers)
    clipman_plugin->menu_item_show_number =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == options->ExitSave)
    clipman_clips->save_on_exit =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == options->PreventEmpty)
    clipman_clips->prevent_empty =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == options->IgnoreSelection)
    clipman_clips->ignore_primary =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == options->IgnoreStatic)
    clipman_clips->ignore_static_clipboard =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  else if (button == options->BehaviourNormal)
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
        clipman_clips->behavior = NORMAL;
    }

  else if (button == options->BehaviourStrictly)
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
        clipman_clips->behavior = STRICTLY;
    }

  else if (button == options->StaticDefault)
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
        clipman_clips->static_selection = DEFAULT;
    }

  else if (button == options->StaticPrimary)
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
        clipman_clips->static_selection = PRIMARY;
    }

  else if (button == options->StaticBoth)
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
        clipman_clips->static_selection = BOTH;
    }

}

