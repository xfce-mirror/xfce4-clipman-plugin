/*  $Id$
 *
 *  Copyright (c) 2006 Nick Schermer <nick@xfce.org>
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

#include <string.h>
#include <gtk/gtk.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "clipman.h"
#include "clipman-dialogs.h"

typedef struct
{
    ClipmanPlugin *clipman;

    GtkWidget     *ExitSave;
    GtkWidget     *IgnoreSelection;
    GtkWidget     *PreventEmpty;

    GtkWidget     *Behaviour;

    GtkWidget     *ItemNumbers;
    GtkWidget     *SeparateMenu;

    GtkWidget     *HistorySize;
    GtkWidget     *ItemChars;
}
ClipmanOptions;

static void
clipman_configure_response (GtkWidget      *dialog,
                            int             response,
                            ClipmanOptions *options)
{
        DBG("Destroy the dialog");

        g_object_set_data (G_OBJECT (options->clipman->plugin), "dialog", NULL);

        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (options->Behaviour)))
            options->clipman->Behaviour = NORMAL;
        else
            options->clipman->Behaviour = STRICTLY;

        if (options->clipman->HistoryItems != gtk_range_get_value (GTK_RANGE (options->HistorySize)))
	{
	    options->clipman->HistoryItems   = gtk_range_get_value (GTK_RANGE (options->HistorySize));
            clipman_check_array_len (options->clipman);
	}

        options->clipman->MenuCharacters = gtk_range_get_value (GTK_RANGE (options->ItemChars));

        clipman_save (options->clipman->plugin, options->clipman);

        clipman_remove_selection_clips (options->clipman);

        xfce_panel_plugin_unblock_menu (options->clipman->plugin);

        gtk_widget_destroy (dialog);

        panel_slice_free (ClipmanOptions, options);
}

static void
set_scale_to_spin (GtkWidget *scalewidget,
                   GtkWidget *spinwidget)
{
    guint value;
    value = gtk_range_get_value (GTK_RANGE (scalewidget));
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinwidget), value);
}

static void
set_spin_to_scale (GtkWidget *spinwidget,
                   GtkWidget *scalewidget)
{
    guint value;
    value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinwidget));
    gtk_range_set_value (GTK_RANGE (scalewidget), value);
}

static void
toggle_button (GtkWidget      *button,
               ClipmanOptions *options)
{
    if (button == options->ExitSave)
        options->clipman->ExitSave =
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    else if (button == options->IgnoreSelection)
    {
        options->clipman->IgnoreSelect =
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

        if (options->clipman->IgnoreSelect)
            gtk_widget_set_sensitive (options->SeparateMenu, FALSE);
        else
            gtk_widget_set_sensitive (options->SeparateMenu, TRUE);
    }
    else if (button == options->PreventEmpty)
        options->clipman->PreventEmpty =
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    else if (button == options->ItemNumbers)
        options->clipman->ItemNumbers =
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    else if (button == options->SeparateMenu)
        options->clipman->SeparateMenu =
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
}

void
clipman_configure (XfcePanelPlugin *plugin,
                   ClipmanPlugin   *clipman)
{
    GtkWidget      *dialog, *dialog_vbox, *frame, *button, *label;
    GtkWidget      *vbox, *hbox, *notebook_vbox, *notebook;
    GtkTooltips    *tooltips;
    ClipmanOptions *options;
    GSList         *group;

    tooltips = gtk_tooltips_new ();

    options = panel_slice_new0 (ClipmanOptions);
    options->clipman = clipman;

    xfce_panel_plugin_block_menu (clipman->plugin);

    dialog = xfce_titled_dialog_new_with_buttons (_("Clipboard Manager"),
                                                  GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                                                  GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                                  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                                  NULL);
/*    xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog),
                                     _("Configure the clipboard manager plugin")); */

    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

    g_object_set_data (G_OBJECT (clipman->plugin), "dialog", dialog);

    dialog_vbox = GTK_DIALOG (dialog)->vbox;

    notebook = gtk_notebook_new ();
    gtk_box_pack_start (GTK_BOX (dialog_vbox), notebook, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), BORDER-3);

    notebook_vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (notebook), notebook_vbox);

    /**
     * The general frame
     **/
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER-3);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);

    button = options->ExitSave = gtk_check_button_new_with_mnemonic (_("Save clipboard contents on _exit"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->ExitSave);

    g_signal_connect (G_OBJECT (button), "toggled",
            G_CALLBACK (toggle_button), options);

    gtk_tooltips_set_tip (tooltips, button,
                          _("Select this option to save the clipboard history when you exit Xfce and "
			    "restore it when you login again."), NULL);

    button = options->IgnoreSelection = gtk_check_button_new_with_mnemonic (_("_Ignore selections"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->IgnoreSelect);

    gtk_tooltips_set_tip (tooltips, button,
                          _("Select this option will prevent Clipman from including the contents of "
			    "the selection in the history."), NULL);

    g_signal_connect (G_OBJECT (button), "toggled",
            G_CALLBACK (toggle_button), options);

    button = options->PreventEmpty = gtk_check_button_new_with_mnemonic (_("Pre_vent empty clipboard"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->PreventEmpty);

    g_signal_connect (G_OBJECT (button), "toggled",
            G_CALLBACK (toggle_button), options);

    gtk_tooltips_set_tip (tooltips, button,
                          _("Select this option to prevent an empty clipboard. The most recent "
			    "history item will be added to the clipboard."), NULL);

    label = gtk_label_new (_("<b>General</b>"));
    gtk_frame_set_label_widget (GTK_FRAME (frame), label);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    gtk_misc_set_padding (GTK_MISC (label), 2, 0);

    /**
     * separate clipboards frame
     **/
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER-3);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);

    group = NULL;

    button = options->Behaviour = gtk_radio_button_new_with_mnemonic (group, _("Normal clipboard _management"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

    gtk_tooltips_set_tip (tooltips, button,
                          _("When you've copied or cut some text and it is already in the "
			    "Clipman history from the selection clipboard,  only the content "
			    "location will be changed.\n\nWhen a history item is clicked, the "
			    "content will be copied to both clipboards."), NULL);

    gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

    if(clipman->Behaviour == NORMAL)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

    button = gtk_radio_button_new_with_mnemonic (group, _("Strictly separate _both clipboards"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

    gtk_tooltips_set_tip (tooltips, button,
                          _("When you've copied or cut some text it will be added to the Clipman history, "
                            "there will also be a duplicate in the history from the selection clipboard."
                            "\n\n"
                            "When a history item is clicked, the content will only be copied to the "
                            "clipboard it origionally came from."
                            "\n\n"
                            "This options will work best when you "
                            "select the \"Separate Clipboards\" option from the Appearance tab."), NULL);

    gtk_radio_button_set_group (GTK_RADIO_BUTTON (button), group);
    group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

    if(clipman->Behaviour == STRICTLY)
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
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER-3);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);

    button = options->ItemNumbers = gtk_check_button_new_with_mnemonic (_("_Show item numbers"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->ItemNumbers);

    g_signal_connect (G_OBJECT (button), "toggled",
            G_CALLBACK (toggle_button), options);

    gtk_tooltips_set_tip (tooltips, button,
                          _("Select this option to show item numbers in the history list."), NULL);

    button = options->SeparateMenu = gtk_check_button_new_with_mnemonic (_("Se_parate clipboards"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clipman->SeparateMenu);

    g_signal_connect (G_OBJECT (button), "toggled",
            G_CALLBACK (toggle_button), options);

    gtk_tooltips_set_tip (tooltips, button,
                          _("Select this option to split up the clipboard and selection history."), NULL);

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
    frame = gtk_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), BORDER-3);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);

    label = gtk_label_new (_("Clipboard history items:"));
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    button = options->HistorySize = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (clipman->HistoryItems, MINHISTORY, MAXHISTORY, 1, 5, 0)));
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 10);
    gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
    gtk_scale_set_digits (GTK_SCALE (button), 0);

    gtk_tooltips_set_tip (tooltips, button,
                          _("The number of items stored in the history list."), NULL);

    button = gtk_spin_button_new_with_range(MINHISTORY, MAXHISTORY, 1);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), clipman->HistoryItems);

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

    button = options->ItemChars = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (clipman->MenuCharacters, MINCHARS, MAXCHARS, 1, 5, 0)));
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 10);
    gtk_scale_set_draw_value (GTK_SCALE (button), FALSE);
    gtk_scale_set_digits (GTK_SCALE (button), 0);

    gtk_tooltips_set_tip (tooltips, button,
                          _("The number of characters showed in the history list. After this amount of characters, the history label will be ellipsized."), NULL);

    button = gtk_spin_button_new_with_range(MINCHARS, MAXCHARS, 1);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), clipman->MenuCharacters);

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

    g_signal_connect(dialog, "response",
        G_CALLBACK(clipman_configure_response), options);

    gtk_widget_show_all (dialog);
}
