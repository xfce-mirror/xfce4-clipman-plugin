/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Nick Schermer <nickschermer@gmail.com>
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

#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "clipman.h"
#include "clipman-dialogs.h"

/**
 * Dialog response
 **/
static void
clipman_configure_response (GtkWidget *dialog, int response, ClipmanOptions *options)
{
	if(response == GTK_RESPONSE_HELP)
	{
		DBG("This will open a the clipman wiki... in the future...");
	}
	else
	{
		DBG("Save the dialog settings");
		
		options->clipman->ExitSave		= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->G_ExitSave));
		options->clipman->IgnoreSelect		= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->G_IgnoreSelection));
		options->clipman->PreventEmpty		= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->G_PreventEmpty));
		options->clipman->FromOneType		= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->G_FromOneType));
	
		options->clipman->ItemNumbers		= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->M_ItemNumbers));
		options->clipman->SeparateBoards	= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->M_SepBoards));
		options->clipman->ColoredItems		= gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->M_ColoredItems));
		
		options->clipman->HistoryItems		= gtk_range_get_value (GTK_RANGE (options->N_HistorySize));
		options->clipman->MenuCharacters	= gtk_range_get_value (GTK_RANGE (options->N_ItemChars));
		
		/* Regenerate the titles and check the array length */
		clipman_regenerate_titles (options->clipman);
		clipman_check_array_len (options->clipman);
		clipman_save (options->clipman->plugin, options->clipman);
		
		xfce_panel_plugin_unblock_menu (options->clipman->plugin);
		gtk_widget_destroy (dialog);
		g_free(options);
	}
	
}

/**
 * Sync Spin button when slider is dragged
 **/
static void
set_scale_to_spin (GtkWidget *scalewidget, GtkWidget *spinwidget)
{
	gint value;
	value = gtk_range_get_value (GTK_RANGE (scalewidget));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinwidget), value);
}

/**
 * Sync slider button when spin button is changed
 **/
static void
set_spin_to_scale(GtkWidget *spinwidget, GtkWidget *scalewidget)
{
	gint value;
	value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinwidget)); 
	gtk_range_set_value (GTK_RANGE (scalewidget), value);
}

/**
 * Disables the radio buttons if toggled
 **/
static void
toggle_prevent_empty (GtkWidget *button, ClipmanOptions *options)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->G_PreventEmpty)))
	{
		gtk_widget_set_sensitive (options->G_FromOneType, TRUE);
		gtk_widget_set_sensitive (options->G_FromOneType_dup, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (options->G_FromOneType, FALSE);
		gtk_widget_set_sensitive (options->G_FromOneType_dup, FALSE);
	}
}

/**
 * Disables some options if selection is ignored
 **/
static void
toggle_ignore_selection (GtkWidget *button, ClipmanOptions *options)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->G_IgnoreSelection)))
	{
		gtk_widget_set_sensitive (options->M_SepBoards, FALSE);	
	}
	else
	{
		gtk_widget_set_sensitive (options->M_SepBoards, TRUE);
	}
}

/**
 * Configure dialog
 **/
void
clipman_configure (XfcePanelPlugin *plugin, ClipmanPlugin *clipman)
{
	GtkWidget *dialog, *dialog_vbox, *header, *frame, *align, *button, *label;
	GtkWidget *vbox, *hbox;
	
	ClipmanOptions *options;
	GSList *group;

	DBG("Show the properties dialog");

	options = g_new0 (ClipmanOptions, 1);
	options->clipman = clipman;
	
	xfce_panel_plugin_block_menu (clipman->plugin);
	
	dialog = gtk_dialog_new_with_buttons (_("Properties"), 
			GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
			GTK_DIALOG_DESTROY_WITH_PARENT |
			GTK_DIALOG_NO_SEPARATOR,
			/* GTK_STOCK_HELP, GTK_RESPONSE_HELP, */
			GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
			NULL);
		gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ALWAYS);
		gtk_window_set_icon_name (GTK_WINDOW (dialog), "gtk-properties");
		gtk_window_set_keep_above(GTK_WINDOW (dialog), TRUE);
		gtk_window_stick (GTK_WINDOW (dialog));
	
	dialog_vbox = GTK_DIALOG (dialog)->vbox;
		gtk_widget_show (dialog_vbox);
		
	header = xfce_create_header (NULL, _("Clipman"));
		gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
		gtk_container_set_border_width (GTK_CONTAINER (header), BORDER-3);
		gtk_widget_show (header);
		gtk_box_pack_start (GTK_BOX (dialog_vbox), header, FALSE, TRUE, 0);
	
	/**
	 * The general frame
	 **/
	frame = gtk_frame_new (NULL);
		gtk_widget_show (frame);
		gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER-3);
		
	vbox = gtk_vbox_new (FALSE, 2);
		gtk_widget_show (vbox);
		gtk_container_add (GTK_CONTAINER (frame), vbox);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
	
	button = options->G_ExitSave = gtk_check_button_new_with_mnemonic (_("Save clipboard contents on _exit"));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->ExitSave);
	
	button = options->G_IgnoreSelection = gtk_check_button_new_with_mnemonic (_("_Ignore selections"));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->IgnoreSelect);
		
		g_signal_connect (G_OBJECT (button), "toggled",
				G_CALLBACK (toggle_ignore_selection), options);
	
	button = options->G_PreventEmpty = gtk_check_button_new_with_mnemonic (_("Pre_vent empty clipboard"));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->PreventEmpty);
		
		g_signal_connect (G_OBJECT (button), "toggled",
				G_CALLBACK (toggle_prevent_empty), options);
		
		group = NULL;
		
		align = gtk_alignment_new (0.5, 0.5, 1, 1);
			gtk_widget_show (align);
			gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
			gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 19, 0);
		
		button = options->G_FromOneType = gtk_radio_button_new_with_mnemonic (group, _("Last clip from same clipboard type"));
			gtk_widget_show (button);
			gtk_container_add (GTK_CONTAINER (align), button);
			
			gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
			group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
			
			if(clipman->FromOneType)
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			
			if (!clipman->PreventEmpty)
			{
				gtk_widget_set_sensitive (button, FALSE);
			}
			
		align = gtk_alignment_new (0.5, 0.5, 1, 1);
			gtk_widget_show (align);
			gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
			gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 19, 0);
		
		button = options->G_FromOneType_dup = gtk_radio_button_new_with_mnemonic (group, _("Last clip added to list"));
			gtk_widget_show (button);
			gtk_container_add (GTK_CONTAINER (align), button);
			
			gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
			group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
			
			if(!clipman->FromOneType)
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			
			if (!clipman->PreventEmpty)
			{
				gtk_widget_set_sensitive (button, FALSE);
			}
			
	label = gtk_label_new (_("<b>General</b>"));
		gtk_widget_show (label);
		gtk_frame_set_label_widget (GTK_FRAME (frame), label);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_misc_set_padding (GTK_MISC (label), 2, 0);
	
	/**
	 * Menu appearance frame
	 **/
	frame = gtk_frame_new (NULL);
		gtk_widget_show (frame);
		gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER-3);
	
	vbox = gtk_vbox_new (FALSE, 2);
		gtk_widget_show (vbox);
		gtk_container_add (GTK_CONTAINER (frame), vbox);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
	
	button = options->M_ItemNumbers = gtk_check_button_new_with_mnemonic (_("_Show item numbers"));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->ItemNumbers);
	
	button = options->M_SepBoards = gtk_check_button_new_with_mnemonic (_("Se_parate clipboards"));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->SeparateBoards);
		
		if (clipman->IgnoreSelect)
		{
			gtk_widget_set_sensitive (button, FALSE);
		}
	
	button = options->M_ColoredItems = gtk_check_button_new_with_mnemonic (_("Use _colored menu items"));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->ColoredItems);
	
	label = gtk_label_new (_("<b>Menu appearance</b>"));
		gtk_widget_show (label);
		gtk_frame_set_label_widget (GTK_FRAME (frame), label);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_misc_set_padding (GTK_MISC (label), 2, 0);
		
	/**
	 * Numbers frame
	 **/
	frame = gtk_frame_new (NULL);
		gtk_widget_show (frame);
		gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, TRUE, 0);
		gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER-3);
	
	vbox = gtk_vbox_new (FALSE, 2);
		gtk_widget_show (vbox);
		gtk_container_add (GTK_CONTAINER (frame), vbox);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
	 
	label = gtk_label_new (_("Clipboard history items:"));
		gtk_widget_show (label);
		gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	
	hbox = gtk_hbox_new (FALSE, 0);
		gtk_widget_show (hbox);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	button = options->N_HistorySize = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (clipman->HistoryItems, MINHISTORY, MAXHISTORY, 1, 5, 0)));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 10);
		gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
		gtk_scale_set_digits (GTK_SCALE (button), 0);
	
	button = gtk_spin_button_new_with_range(MINHISTORY, MAXHISTORY, 1);
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), clipman->HistoryItems);
		
		/* Sync history widgets */
		g_signal_connect (G_OBJECT (options->N_HistorySize), "value_changed",
				G_CALLBACK (set_scale_to_spin), button);
		
		g_signal_connect (G_OBJECT (button), "value_changed",
				G_CALLBACK (set_spin_to_scale), options->N_HistorySize);
	
	label = gtk_label_new (_("Menu item characters:"));
		gtk_widget_show (label);
		gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	
	hbox = gtk_hbox_new (FALSE, 0);
		gtk_widget_show (hbox);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	button = options->N_ItemChars = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (clipman->MenuCharacters, MINCHARS, MAXCHARS, 1, 5, 0)));
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 10);
		gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
		gtk_scale_set_digits (GTK_SCALE (button), 0);
	
	button = gtk_spin_button_new_with_range(MINCHARS, MAXCHARS, 1);
		gtk_widget_show (button);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), clipman->MenuCharacters);
		
		g_signal_connect (G_OBJECT (options->N_ItemChars), "value_changed",
				G_CALLBACK (set_scale_to_spin), button);
	
		g_signal_connect (G_OBJECT (button), "value_changed",
				G_CALLBACK (set_spin_to_scale), options->N_ItemChars);
	
	label = gtk_label_new (_("<b>Numbers</b>"));
		gtk_widget_show (label);
		gtk_frame_set_label_widget (GTK_FRAME (frame), label);
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_misc_set_padding (GTK_MISC (label), 4, 0);
	
	g_signal_connect(dialog, "response",
			G_CALLBACK(clipman_configure_response), options);
	
	gtk_widget_show (dialog);
}

/**
 * This shows the about dialog
 **/
void
clipman_about (XfcePanelPlugin *plugin)
{
	XfceAboutInfo	*about;
	GtkWidget	*dialog;
	GdkPixbuf	*image;

	DBG("Show the about dialog");
	
	about = xfce_about_info_new (_("Clipman"), "",
		_("Clipboard manager for the Xfce desktop"), 
		XFCE_COPYRIGHT_TEXT ("2005", "Nick Schermer"),
		XFCE_LICENSE_GPL);
	xfce_about_info_set_homepage (about, "http://www.xfce.org");
	xfce_about_info_add_credit (about, "Nick Schermer", "nickschermer@gmail.com", _("Developer"));
	xfce_about_info_add_credit (about, "Eduard Roccatello", "eduard@xfce.org", _("Developer"));
	
	image = xfce_themed_icon_load ("gtk-paste", 32);
	dialog = xfce_about_dialog_new (NULL, about, image);
	g_object_unref (image);
	
	gtk_widget_set_size_request (dialog, 400, 300);
	xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (dialog));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	xfce_about_info_free (about);
}
