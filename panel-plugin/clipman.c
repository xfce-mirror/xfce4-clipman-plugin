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

#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "clipman.h"
#include "clipman-dialogs.h"

static void clipman_construct (XfcePanelPlugin *plugin);

/* Register Plugin */
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (clipman_construct);

/**
 * Free an array clip
 **/
static void
clipman_free_clip (ClipmanClip *clip)
{
	DBG("...");
	
	g_free (clip->text);
	g_free (clip->title);
	
	g_free (clip);
}

/**
 * Destroys the menu when you don't click a menu item
 * but somewhere else in the screen when the menu was poped up
 **/
static void
clipman_destroy_menu (GtkWidget *menu, ClipmanPlugin *clipman)
{
	DBG("...");
	
	if(menu)
		gtk_widget_destroy (menu);
	if(clipman->menu)
		gtk_widget_destroy (clipman->menu);
}

/**
 * Clear the entire array content, but
 * does not remove the array
 **/
static void
clipman_clear (GtkWidget *mi, GdkEventButton *ev, ClipmanPlugin *clipman)
{
	DBG("...");
	
	if (xfce_confirm(N_("Are you sure you want to clear the history?"), "gtk-yes", NULL))
	{
		gtk_clipboard_set_text(primaryClip, "", -1);
		gtk_clipboard_set_text(defaultClip, "", -1);
		
		while (clipman->clips->len > 0)
		{
			ClipmanClip *clip = g_ptr_array_index (clipman->clips, 0);
			g_ptr_array_remove (clipman->clips, clip);
			clipman_free_clip (clip);
		}
	}
	gtk_widget_destroy (clipman->menu);
}

/**
 * Check if there are not more arrays in the 
 * clipboard then allowed. If so remove the oldest ones
 **/
void
clipman_check_array_len (ClipmanPlugin *clipman)
{
	DBG("...");
	
	while (clipman->clips->len > clipman->HistoryItems)
	{
		ClipmanClip *clip = g_ptr_array_index (clipman->clips, 0);
		g_ptr_array_remove (clipman->clips, clip);
		clipman_free_clip (clip);
		
		DBG("A clip hase been removed");
	}
}

/**
 * Creates a proper title
 **/
static gchar *
clipman_create_title (gchar *txt, gint chars)
{
	txt = g_strndup(txt, chars);
	guint i = 0;
	while (txt[i] != '\0') {
		if (txt[i] == '\n' || txt[i] == '\r' || txt[i] == '\t' || txt[i] == '<' || txt[i] == '>' || txt[i] == '&')
			txt[i] = ' ';
		i++;
	}
	txt = g_strstrip(txt);
	return txt;
}

/**
 * Create all titles again, this function is
 * called when you change the menu items characters value
 **/
void
clipman_regenerate_titles (ClipmanPlugin *clipman)
{
	gint i;
	
	for (i=0; i < clipman->clips->len; i++)
	{
		ClipmanClip *clip = g_ptr_array_index (clipman->clips, i);
		
		clip->title = clipman_create_title (clip->text, clipman->MenuCharacters);
	}
}

/**
 * Create a clip and adds it to the array
 **/
static void
clipman_add_clip (ClipmanPlugin *clipman, gchar *txt, ClipboardType type)
{
	DBG("...");
	
	if (txt != "")
	{
		ClipmanClip *new_clip;
		new_clip = g_new0 (ClipmanClip, 1);
		
		new_clip->text		= g_strdup (txt);
		new_clip->title		= clipman_create_title (txt, clipman->MenuCharacters);
		new_clip->fromtype	= type;
		
		g_ptr_array_add (clipman->clips, new_clip);
		
		DBG("Clips %d/%d", clipman->clips->len, clipman->HistoryItems);
	}
}

/**
 * Check if text is already
 * somewhere in the clipboard array
 **/
static gboolean
clipman_exists (ClipmanPlugin *clipman, gchar *txt, ClipboardType type)
{
	gint i;
	
	/* Walk through the array backwards, because
	 * if the text exists, this will probably be the newest */

	for (i = (clipman->clips->len - 1); i >= 0; i--)
	{
		ClipmanClip *clip = g_ptr_array_index (clipman->clips, i);
		
		if ((clip->text != NULL) && (strcmp(clip->text, txt) == 0))
		{
			if (type == FROM_DEFAULT && clip->fromtype == FROM_PRIMIRY)
				clip->fromtype = FROM_DEFAULT;
			
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Copy the clicked menu item the the clipboard (mouse button 1)
 * Remove the clip from the array if confirmed (button 3)
 **/
static void
clipman_item_clicked (GtkWidget *mi, GdkEventButton *ev, ClipmanAction *action)
{
	DBG("...");
	
	gtk_widget_destroy (action->clipman->menu);
	
	if (ev->button == 1)
	{
		DBG("Clip copied to the clipboards");
		gtk_clipboard_set_text(defaultClip, action->clip->text, -1);
		gtk_clipboard_set_text(primaryClip, action->clip->text, -1);
	}
	else if (ev->button == 3)
	{
		if (xfce_confirm(N_("Are you sure you want to remove this item from the clipboard?"), "gtk-yes", NULL))
		{
			/* Only remove the clip from the clipboard if it's the same */
			if ( gtk_clipboard_wait_for_text (defaultClip) &&
			     !strcmp(gtk_clipboard_wait_for_text (defaultClip), action->clip->text)
			   )
			{
				gtk_clipboard_set_text(defaultClip, "", -1);
			}
				
			if ( gtk_clipboard_wait_for_text (primaryClip) &&
			     !strcmp(gtk_clipboard_wait_for_text (primaryClip), action->clip->text)
			   )
			{
				gtk_clipboard_set_text(primaryClip, "", -1);
			}
			
			/* Remove the clip from the array */
			g_ptr_array_remove (action->clipman->clips, action->clip);
			clipman_free_clip (action->clip);
		}
	}
}

/**
 * Create a nice looking menu item, with colors and stuff
 * of couse you can disable all this eye-candy in the properties window
 **/
GtkWidget *
clipman_create_menuitem (ClipmanAction *action, gint number, gboolean bold)
{
	GtkWidget *mi = gtk_menu_item_new_with_label  ("");
	gchar *title;
	
	if(action->clipman->ColoredItems)
	{
		switch (action->clip->fromtype)
		{
			case FROM_PRIMIRY:
				title = g_strdup_printf("<span foreground=\"%s\">%s</span>", action->clipman->PriColor, action->clip->title);
				break;
			case FROM_DEFAULT:
				title = g_strdup_printf("<span foreground=\"%s\">%s</span>", action->clipman->DefColor, action->clip->title);
				break;
		}
	}
	else
	{
		title = g_strdup_printf("%s", action->clip->title);
	}
	
	if (bold)
	{
		title = g_strdup_printf("<b>%s</b>", title);
	}
	
	if(action->clipman->ItemNumbers)
	{
		if(action->clipman->ColoredItems)
		{
			if(number < 10)
				title = g_strdup_printf("<tt><span foreground=\"%s\" size=\"smaller\">%d. </span></tt> %s", action->clipman->NumColor, number ,title);
			else
				title = g_strdup_printf("<tt><span foreground=\"%s\" size=\"smaller\">%d.</span></tt> %s",  action->clipman->NumColor, number ,title);
		}
		else
		{
			if(number < 10)
				title = g_strdup_printf("<tt><span size=\"smaller\">%d. </span></tt> %s", number ,title);
			else
				title = g_strdup_printf("<tt><span size=\"smaller\">%d.</span></tt> %s",  number ,title);
		}
	}
	
	gtk_label_set_markup(GTK_LABEL(GTK_BIN(mi)->child), title);
	gtk_widget_show (mi);
	
	return mi;
}

/**
 * Builds the menu if you want a separated menu layout
 **/
static void
clipman_clicked_separated (GtkMenu *menu, ClipmanPlugin *clipman)
{
	DBG("...");
	
	gchar *priClip, *defClip;
	gint i, j;
	ClipmanAction *action = NULL;
	GtkWidget *mi;
	
	defClip = gtk_clipboard_wait_for_text (defaultClip);
	j = 0;
	
	for (i = (clipman->clips->len - 1); i >= 0; i--)
	{
		ClipmanClip *clip = g_ptr_array_index (clipman->clips, i);
		
		if(clip->fromtype == FROM_DEFAULT)
		{
			j++;
			
			action = g_new0 (ClipmanAction, 1);
			action->clipman = clipman;
			action->clip = clip;
			
			if((defClip != NULL) && (clip->text != NULL) && (strcmp(clip->text, defClip) == 0))
			{
				mi = clipman_create_menuitem (action, j, TRUE);
				defClip = NULL;
			}
			else
			{
				mi = clipman_create_menuitem (action, j, FALSE);
			}
			
			g_signal_connect (G_OBJECT(mi), "button_release_event",
					G_CALLBACK(clipman_item_clicked), action);
			
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
		}
	}
	
	g_free (defClip);

	if (j == 0)
	{
		mi = gtk_menu_item_new_with_label (N_("< Default History Empty >"));
			gtk_widget_show (mi);
			gtk_widget_set_sensitive (mi, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	}
	
	if (!clipman->IgnoreSelect)
	{
		mi = gtk_separator_menu_item_new ();
			gtk_widget_show (mi);
			gtk_widget_set_sensitive (mi, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
		
		priClip = gtk_clipboard_wait_for_text (primaryClip);
		j = 0;
		
		for (i = (clipman->clips->len - 1); i >= 0; i--)
		{
			ClipmanClip *clip = g_ptr_array_index (clipman->clips, i);
			
			if(clip->fromtype == FROM_PRIMIRY)
			{
				j++;
				
				action = g_new0 (ClipmanAction, 1);
				action->clipman = clipman;
				action->clip = clip;
				
				if((priClip != NULL) && (clip->text != NULL) && (strcmp(clip->text, priClip) == 0))
				{
					mi = clipman_create_menuitem (action, j, TRUE);
					priClip = NULL;
				}
				else
				{
					mi = clipman_create_menuitem (action, j, FALSE);
				}
				
				g_signal_connect (G_OBJECT(mi), "button_release_event",
						G_CALLBACK(clipman_item_clicked), action);
				
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
			}
		}
		
		if (j == 0)
		{
			mi = gtk_menu_item_new_with_label (N_("< Selection History Empty >"));
				gtk_widget_show (mi);
				gtk_widget_set_sensitive (mi, FALSE);
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
		}
		
		g_free (priClip);
	}
}

/**
 * Builds the menu if you don't want a separated menu layout
 **/
static void
clipman_clicked_not_separated (GtkMenu *menu, ClipmanPlugin *clipman)
{
	DBG("...");
	
	gchar *priClip, *defClip;
	gint i;
	ClipmanAction *action = NULL;
	GtkWidget *mi;
	
	priClip = gtk_clipboard_wait_for_text (primaryClip);
	defClip = gtk_clipboard_wait_for_text (defaultClip);
	
	for (i = (clipman->clips->len - 1); i >= 0; i--)
	{
		ClipmanClip *clip = g_ptr_array_index (clipman->clips, i);
		
		action = g_new0 (ClipmanAction, 1);
		action->clipman = clipman;
		action->clip = clip;
		
		if((defClip != NULL) && (clip->text != NULL) && (strcmp(clip->text, defClip) == 0))
		{
			mi = clipman_create_menuitem (action, clipman->clips->len-i, TRUE);
			priClip = NULL;
		}
		else if((priClip != NULL) && (clip->text != NULL) && (strcmp(clip->text, priClip) == 0))
		{
			mi = clipman_create_menuitem (action, clipman->clips->len-i, TRUE);
			priClip = NULL;
		}
		else
		{
			mi = clipman_create_menuitem (action, clipman->clips->len-i, FALSE);
		}
		
		g_signal_connect (G_OBJECT(mi), "button_release_event",
				G_CALLBACK(clipman_item_clicked), action);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	}
	
	g_free (priClip);
	g_free (defClip);
}

/**
 * This function build the menu when the button is clicked
 **/
static void
clipman_clicked (GtkWidget *button, ClipmanPlugin *clipman)
{
	DBG("...");
	
	GtkWidget *mi;
	GtkMenu	*menu;
	gchar *title;
	
	/**
	 * Optional, see if the plugin is blocked:
	 * g_object_get_data (G_OBJECT (clipman->plugin), "xfce-panel-plugin-block") == GINT_TO_POINTER(1);
	 **/
	
	menu = GTK_MENU(gtk_menu_new());

	title = g_strdup_printf("<span weight=\"bold\">%s</span>", N_("Clipman History"));
	mi = gtk_menu_item_new_with_label  ("");
		gtk_widget_show (mi);
		gtk_label_set_markup(GTK_LABEL(GTK_BIN(mi)->child), title);
		gtk_widget_set_sensitive (mi, FALSE);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	
	mi = gtk_separator_menu_item_new ();
		gtk_widget_show (mi);
		gtk_widget_set_sensitive (mi, FALSE);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	
	if(clipman->SeparateBoards && !clipman->IgnoreSelect && clipman->clips->len > 0)
	{
		clipman_clicked_separated (menu, clipman);
		
		mi = gtk_separator_menu_item_new ();
			gtk_widget_show (mi);
			gtk_widget_set_sensitive (mi, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
		
		mi = gtk_menu_item_new_with_label (N_("Clear Clipman"));
			gtk_widget_show (mi);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
			
			g_signal_connect (G_OBJECT (mi), "button_release_event",
				G_CALLBACK (clipman_clear), clipman);
	}
	else if (clipman->clips->len > 0)
	{
		clipman_clicked_not_separated (menu, clipman);
		
		mi = gtk_separator_menu_item_new ();
			gtk_widget_show (mi);
			gtk_widget_set_sensitive (mi, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	
		mi = gtk_menu_item_new_with_label (N_("Clear Clipman"));
			gtk_widget_show (mi);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
		
			g_signal_connect (G_OBJECT (mi), "button_release_event",
				G_CALLBACK (clipman_clear), clipman);
	}
	else
	{
		mi = gtk_menu_item_new_with_label (N_("< Clipboard Empty >"));
			gtk_widget_show (mi);
			gtk_widget_set_sensitive (mi, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	}
	
	clipman->menu = GTK_WIDGET(menu);
	
	/* Also destroy the menu items when nothing is clicked */
	g_signal_connect (G_OBJECT(menu), "deactivate",
		G_CALLBACK(clipman_destroy_menu), clipman);
	
	gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

/**
 * This function will full the clipboard when it's empty
 **/
static void
clipman_restore_empty (ClipmanPlugin *clipman, ClipboardType type)
{
	if (clipman->FromOneType)
	{
		gint i;
		
		/* Walk through the array till a clip of it's own 
		 * type is found, then past it in the clipboard */
		for (i = (clipman->clips->len - 1); i >= 0; i--)
		{
			ClipmanClip *clip = g_ptr_array_index (clipman->clips, i);
			
			if (clip->fromtype == type)
			{
				switch (type)
				{
					case FROM_PRIMIRY:
						gtk_clipboard_set_text(primaryClip, clip->text, -1);
						break;
					case FROM_DEFAULT:
						gtk_clipboard_set_text(defaultClip, clip->text, -1);
						break;
				}
				
				DBG("Item restored with a clip from it's own type");
				
				break;
			}
		}
	}
	else
	{
		/* Grap the latest clip and past it in the clipboard */
		if (clipman->clips->len > 0)
		{
			ClipmanClip *clip = g_ptr_array_index (clipman->clips, (clipman->clips->len-1));
			
			switch (type)
			{
				case FROM_PRIMIRY:
					gtk_clipboard_set_text(primaryClip, clip->text, -1);
					break;
				case FROM_DEFAULT:
					gtk_clipboard_set_text(defaultClip, clip->text, -1);
					break;
			}
			
			DBG("Last clip added");
		}
	}	
}

/**
 * Checks the 2 clipboards every x miliseconds
 **/
static gboolean
clipman_check (ClipmanPlugin *clipman)
{
	/* Get mouse button information */
	GdkModifierType state;
	gchar *txt = NULL;
	gdk_window_get_pointer(NULL, NULL, NULL, &state);
	
	xfce_panel_plugin_block_menu (clipman->plugin);
	
	/* We ignore the selection clipboard entirely if you've activated this in the options dialog */
	if(!clipman->IgnoreSelect)
	{
		txt = gtk_clipboard_wait_for_text (primaryClip);
		
		if(clipman->PreventEmpty && txt == NULL)
		{
			clipman_restore_empty (clipman, FROM_PRIMIRY);
		}
		else if (txt != NULL)
		{
			if(!(state & GDK_BUTTON1_MASK) && !clipman_exists(clipman, txt, FROM_PRIMIRY))
			{
				DBG("Item added from primairy clipboard");
				clipman_add_clip (clipman, txt, FROM_PRIMIRY);
				clipman_check_array_len (clipman);
			}
			
			g_free(txt);
			txt = NULL;
		}
	}
	
	txt = gtk_clipboard_wait_for_text (defaultClip);

	if(clipman->PreventEmpty && txt == NULL)
	{
		clipman_restore_empty (clipman, FROM_DEFAULT);
	}
	else if (txt != NULL)
	{
		if (!clipman_exists(clipman, txt, FROM_DEFAULT))
		{
			DBG("Item added from default clipboard");
			clipman_add_clip (clipman, txt, FROM_DEFAULT);
			clipman_check_array_len (clipman);
		}
		g_free(txt);
		txt = NULL;
	}
	
	xfce_panel_plugin_unblock_menu (clipman->plugin);
	
	return TRUE;
}

/**
 * Some function to restart the clipboard loop if it stops
 **/
static void
clipman_reset_timer (ClipmanPlugin *clipman)
{
	DBG("...");
	
	if (!(clipman->killTimeout))
	{
		if (clipman->timeId != 0)
			g_source_remove(clipman->timeId);
		
		clipman->timeId = g_timeout_add_full(G_PRIORITY_LOW, TIMER_INTERVAL,
			(GSourceFunc)clipman_check, clipman, (GDestroyNotify)clipman_reset_timer);
	}
}

/**
 * Saves the clipboard settings and if allowed the clipboard history
 **/
void
clipman_save (XfcePanelPlugin *plugin, ClipmanPlugin *clipman)
{
	DBG("...");
	
	XfceRc	*rc;
	gchar	*file;
	gint	i;
	
	/* Open Clipman RC file */
	file = xfce_panel_plugin_save_location (plugin, FALSE);
	DBG("Save to file: %s", file);
	rc = xfce_rc_simple_open (file, FALSE);
	g_free (file);
	
	/* Save the preferences */
	xfce_rc_set_group (rc, "Properties");
	
	xfce_rc_write_bool_entry (rc, "ExitSave",	clipman->ExitSave);
	xfce_rc_write_bool_entry (rc, "IgnoreSelect",	clipman->IgnoreSelect);
	xfce_rc_write_bool_entry (rc, "PreventEmpty",	clipman->PreventEmpty);
	xfce_rc_write_bool_entry (rc, "FromOneType",	clipman->FromOneType);
	
	xfce_rc_write_bool_entry (rc, "ItemNumbers",	clipman->ItemNumbers);
	xfce_rc_write_bool_entry (rc, "SeparateBoards",	clipman->SeparateBoards);
	xfce_rc_write_bool_entry (rc, "ColoredItems",	clipman->ColoredItems);
	
	xfce_rc_write_int_entry	 (rc, "HistoryItems", 	clipman->HistoryItems);
	xfce_rc_write_int_entry	 (rc, "MenuCharacters", clipman->MenuCharacters);

	xfce_rc_write_entry 	 (rc, "DefColor",	clipman->DefColor);
	xfce_rc_write_entry 	 (rc, "PriColor",	clipman->PriColor);
	xfce_rc_write_entry 	 (rc, "NumColor",	clipman->NumColor);
	
	/* Remove old content and create a new one */
	xfce_rc_delete_group (rc, "Clips", TRUE );
	xfce_rc_set_group    (rc, "Clips");
	
	if (clipman->ExitSave && clipman->clips->len > 0)
	{
		DBG("Saving the clipboard history");
		
		gchar name[13];
		
		xfce_rc_write_int_entry	 (rc, "ClipsLen", clipman->clips->len);
		
		for (i = 0; i < clipman->clips->len; ++i)
		{
			ClipmanClip *clip = g_ptr_array_index (clipman->clips, i);
			
			g_snprintf (name, 13, "clip_%d_text", i);
			xfce_rc_write_entry (rc, name, clip->text);
			
			g_snprintf (name, 13, "clip_%d_from", i);
			if (clip->fromtype == FROM_PRIMIRY)
			{
				xfce_rc_write_int_entry (rc, name, 0);
			}
			else
			{
				xfce_rc_write_int_entry (rc, name, 1);
			}
		}
	}

	xfce_rc_close (rc);
}

/**
 * Restores the clipboard settings and if allowed the clipboard hisrtory
 **/
static void
clipman_read (ClipmanPlugin *clipman)
{
	XfceRc	*rc;
	gchar	*file;
	gint	i, clipslen;
	
	file = xfce_panel_plugin_save_location (clipman->plugin, FALSE);
	DBG("Read from file: %s", file);
	rc = xfce_rc_simple_open (file, FALSE);
	g_free (file);
	
	xfce_rc_set_group (rc, "Properties");
	
	clipman->ExitSave 	= xfce_rc_read_bool_entry (rc, "ExitSave",	 DEFEXITSAVE);
	clipman->IgnoreSelect 	= xfce_rc_read_bool_entry (rc, "IgnoreSelect",	 DEFIGNORESELECT);
	clipman->PreventEmpty 	= xfce_rc_read_bool_entry (rc, "PreventEmpty",	 DEFPREVENTEMPTY);
	clipman->FromOneType 	= xfce_rc_read_bool_entry (rc, "FromOneType",	 DEFFROMONETYPE);
	
	clipman->ItemNumbers 	= xfce_rc_read_bool_entry (rc, "ItemNumbers",	 DEFITEMNUMBERS);
	clipman->SeparateBoards = xfce_rc_read_bool_entry (rc, "SeparateBoards", DEFSEPARATEBOARDS);
	clipman->ColoredItems 	= xfce_rc_read_bool_entry (rc, "ColoredItems",	 DEFCOLOREDITEMS);
	
	clipman->HistoryItems 	= xfce_rc_read_int_entry  (rc, "HistoryItems",   DEFHISTORY);
	clipman->MenuCharacters = xfce_rc_read_int_entry  (rc, "MenuCharacters", DEFCHARS);
	
	if (clipman->HistoryItems > MAXHISTORY)
		clipman->HistoryItems = MAXHISTORY;
	if (clipman->HistoryItems < MINHISTORY)
		clipman->HistoryItems = MINHISTORY;
	
	if (clipman->MenuCharacters > MAXCHARS)
		clipman->MenuCharacters = MAXCHARS;
	if (clipman->MenuCharacters < MINCHARS)
		clipman->MenuCharacters = MINCHARS;

	clipman->DefColor = (gchar *)xfce_rc_read_entry (rc, "DefColor", DEFDEFCOLOR);
	clipman->PriColor = (gchar *)xfce_rc_read_entry (rc, "PriColor", DEFPRICOLOR);
	clipman->NumColor = (gchar *)xfce_rc_read_entry (rc, "NumColor", DEFNUMCOLOR);
	
	xfce_rc_set_group (rc, "Clips");
	clipslen = xfce_rc_read_int_entry (rc, "ClipsLen", 0);
	
	if (clipslen > MAXHISTORY)
		clipslen = MAXHISTORY;
	
	if (clipman->ExitSave && clipslen > 0)
	{
		DBG("Restoring the clipboard");
		
		gchar name[13];
		gint type;
		const gchar *value;
		
		for (i = 0; i < clipslen; ++i)
		{
			g_snprintf (name, 13, "clip_%d_text", i);
			value = xfce_rc_read_entry (rc, name, "");
			
			g_snprintf (name, 13, "clip_%d_from", i);
			type = xfce_rc_read_int_entry (rc, name, 0);
			
			if (type == 0)
			{
				clipman_add_clip (clipman, (gchar *)value, FROM_PRIMIRY);
			}
			else
			{
				clipman_add_clip (clipman, (gchar *)value, FROM_DEFAULT);
			}
		}
	}
}

/**
 * Build the clipboard button and
 * defines the varaibles. The clipboard restore
 * function is also called from here.
 **/
static ClipmanPlugin *
clipman_new (XfcePanelPlugin *plugin)
{
	DBG("...");
	
	GdkPixbuf *icon;
	
	ClipmanPlugin *clipman;
	clipman = g_new0 (ClipmanPlugin, 1);
	
	clipman->clips = g_ptr_array_new ();
	clipman->plugin = plugin;
	
	clipman->tooltip = gtk_tooltips_new ();
	g_object_ref (clipman->tooltip);
	
	clipman_read (clipman);
	
	clipman->button = xfce_iconbutton_new ();
		gtk_widget_show (clipman->button);
		gtk_button_set_relief (GTK_BUTTON (clipman->button), GTK_RELIEF_NONE);
		gtk_button_set_focus_on_click (GTK_BUTTON (clipman->button), FALSE);
	
	icon = gtk_widget_render_icon (clipman->button, GTK_STOCK_PASTE, 
                                       GTK_ICON_SIZE_DIALOG, NULL);
		xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (clipman->button), icon);
		g_object_unref (icon);
	
	gtk_tooltips_set_tip (GTK_TOOLTIPS(clipman->tooltip), clipman->button, _("Clipboard Manager"), NULL);
	
	g_signal_connect(clipman->button, "clicked",
			G_CALLBACK(clipman_clicked), clipman);
	
	/* Start the clipman_check function */
	clipman->timeId = g_timeout_add_full(G_PRIORITY_LOW, TIMER_INTERVAL,
		(GSourceFunc)clipman_check, clipman, (GDestroyNotify)clipman_reset_timer);
			
	/* Connect to the clipboards */
	defaultClip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	primaryClip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
			
	return clipman;
}

/**
 * Free the array and remove the structure
 * This will only run when clipman is closed
 **/
static void
clipman_free (ClipmanPlugin *clipman)
{
	DBG("...");
	
	gint i;
	
	g_object_unref (clipman->tooltip);
	
	/* Stop the check loop */
	clipman->killTimeout = TRUE;
	if (clipman->timeId != 0)
		g_source_remove(clipman->timeId);
	
	/* Remove clipboard items */
	for (i = 0; i < clipman->clips->len; ++i)
	{
		ClipmanClip *clip = g_ptr_array_index (clipman->clips, i);
		clipman_free_clip (clip);
	}
	
	g_ptr_array_free (clipman->clips, TRUE);
	
	if (clipman->menu)
		gtk_widget_destroy (clipman->menu);
	
	g_free (clipman);
}

/**
 * Changes the button size when the panel
 * was resized
 **/
static gboolean
clipman_set_size (XfcePanelPlugin *plugin, gint wsize, ClipmanPlugin *clipman)
{
	DBG("...");
	
	gtk_widget_set_size_request (clipman->button, wsize, wsize);

	return TRUE;
}

/**
 * Initialize the plugin
 **/
static void 
clipman_construct (XfcePanelPlugin *plugin)
{
	DBG("...");
	
	ClipmanPlugin *clipman = clipman_new (plugin);
	
	gtk_container_add (GTK_CONTAINER (plugin), clipman->button);
	
	xfce_panel_plugin_add_action_widget (plugin, clipman->button);
	
	g_signal_connect (plugin, "free-data", 
		G_CALLBACK (clipman_free), clipman);
	
	g_signal_connect (plugin, "save", 
		G_CALLBACK (clipman_save), clipman);
	
	g_signal_connect (plugin, "size-changed",
		G_CALLBACK (clipman_set_size), clipman);
	
	xfce_panel_plugin_menu_show_about (plugin);
	g_signal_connect (plugin, "about", 
		G_CALLBACK (clipman_about), NULL);
	
	xfce_panel_plugin_menu_show_configure (plugin);
	g_signal_connect (plugin, "configure-plugin", 
		G_CALLBACK (clipman_configure), clipman);
}
