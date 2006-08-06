/* vim: set expandtab ts=8 sw=4: */

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
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>

#include "clipman.h"
#include "clipman-dialogs.h"

/* The clipboards */
static GtkClipboard *primaryClip;
static GtkClipboard *defaultClip;

/* Register the plugin */
static void
clipman_construct (XfcePanelPlugin *plugin);

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (clipman_construct);

static void
clipman_free_clip (ClipmanClip *clip)
{
    g_free (clip->text);
    g_free (clip->title);
    
    g_free (clip);

    DBG ("Clip successfully freed");
}

static void
clipman_destroy_menu (GtkWidget     *menu,
                      ClipmanPlugin *clipman)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (clipman->button), FALSE);
    
    gtk_widget_destroy (menu);

    DBG ("Menu Destroyed");
}

static gboolean
clipman_clear (GtkWidget      *mi,
               GdkEventButton *ev,
               ClipmanPlugin  *clipman)
{
    ClipmanClip *clip;

    if (xfce_confirm (_("Are you sure you want to clear the history?"), 
	              "gtk-yes", 
                      NULL))
    {
        gtk_clipboard_clear (primaryClip);
	gtk_clipboard_clear (defaultClip);
        
        while (clipman->clips->len > 0)
        {
            clip = g_ptr_array_index (clipman->clips, 0);
            g_ptr_array_remove (clipman->clips, clip);
            clipman_free_clip (clip);
        }
    }
    
    /* 'Save' the empty clipboard */    
    clipman_save (clipman->plugin, clipman);
    
    return FALSE;
}

void
clipman_check_array_len (ClipmanPlugin *clipman)
{
    ClipmanClip *clip;

    DBG ("Checking Array Length");

    while (clipman->clips->len > clipman->HistoryItems)
    {
        clip = g_ptr_array_index (clipman->clips, 0);
        g_ptr_array_remove (clipman->clips, clip);
        clipman_free_clip (clip);
        
        DBG("A clip have been removed");
    }
}

static gchar *
clipman_create_title (gchar *txt,
                      gint   length)
{
    gchar *s, *t, *u;
    
    s = g_strndup (txt, length*8);

    if (!g_utf8_validate (s, -1, NULL))
    {
	DBG ("Title is not utf8 complaint, we're going to convert it");
	
        u = g_locale_to_utf8 (s, -1, NULL, NULL, NULL);
	g_free (s);
	s = u;
	
	/* Check the title again */
	if (!g_utf8_validate (s, -1, NULL))
	{
	    DBG ("Title is still not utf8 complaint, we going to drop this clip");
	    g_free (s);
	    return NULL;
	}
    }

    g_strstrip (s);
    
    t = g_markup_escape_text (s, -1);
    g_free (s);
    
    return t;
}

void
clipman_remove_selection_clips (ClipmanPlugin *clipman)
{
    ClipmanClip *clip;
    guint        i;

    if (!clipman->IgnoreSelect)
        return;
    
    DBG ("Cleaning up all selection clips");
    
    for (i = clipman->clips->len; i--; )
    {
        clip = g_ptr_array_index (clipman->clips, i);
    
        if (clip->fromtype == PRIMARY)
        {
             g_ptr_array_remove (clipman->clips, clip);
             clipman_free_clip (clip);
        }
    }
}

static void
clipman_add_clip (ClipmanPlugin *clipman,
                  gchar         *txt,
                  ClipboardType  type)
{
    ClipmanClip *new_clip;
    
    if (txt != "")
    {
        new_clip = g_new0 (ClipmanClip, 1);
        
        new_clip->title    = clipman_create_title (txt,
                                                   clipman->MenuCharacters);
	
	/* No valid title could be created, drop it... */
	if (new_clip->title == NULL)
	{
	    g_free (new_clip);
	    return;
	}
	
	new_clip->text     = g_strdup (txt);
        new_clip->fromtype = type;
        
        g_ptr_array_add (clipman->clips, new_clip);
        
        DBG("Added clip %d of %d", clipman->clips->len, clipman->HistoryItems);
    }
}

static gboolean
clipman_exists (ClipmanPlugin *clipman,
                gchar         *txt,
                ClipboardType  type)
{
    guint        i;
    ClipmanClip *clip;
    
    /* Walk through the array backwards, because
     * if the text exists, this will probably be the newest */

    for (i = clipman->clips->len; i--; )
    {
        clip = g_ptr_array_index (clipman->clips, i);
        
        if (G_LIKELY ((clip->text != NULL) && 
                      (strcmp(clip->text, txt) == 0))
           )
        {
            switch (clipman->Behaviour)
            {
                case NORMAL:
                    if (type == DEFAULT && 
                        clip->fromtype == PRIMARY)
                            clip->fromtype = DEFAULT;    
                
                    return TRUE;
                
                case STRICTLY:
                    if (type == clip->fromtype)
                        return TRUE;
                    
                    return FALSE;
            }
        }
    }
    
    return FALSE;
}

static gboolean
clipman_item_clicked (GtkWidget      *mi,
                      GdkEventButton *ev,
                      ClipmanAction  *action)
{
    gchar *dtext, *ptext;
    
    if (ev->button == 1 && action->clipman->Behaviour == STRICTLY)
    {
        DBG("Clip copied to his own clipboard (STRICTLY)");
        
        if (action->clip->fromtype == DEFAULT)
	{
	    gtk_clipboard_clear (defaultClip);
            gtk_clipboard_set_text (defaultClip, action->clip->text, -1);
	}
        
        if (action->clip->fromtype == PRIMARY)
	{
	    gtk_clipboard_clear (primaryClip);
            gtk_clipboard_set_text (primaryClip, action->clip->text, -1);
	}
    }
    else if (ev->button == 1)
    {
        gtk_clipboard_clear (defaultClip);
        gtk_clipboard_set_text (defaultClip, action->clip->text, -1);
	DBG ("Clip copied to default clipboard");
        
	if (!action->clipman->IgnoreSelect)
	{
	    gtk_clipboard_clear (primaryClip);
	    gtk_clipboard_set_text (primaryClip, action->clip->text, -1);
	    DBG ("Clip copied to primary clipboard");
	}
    }
    else if (ev->button == 3)
    {
        if (xfce_confirm (_("Are you sure you want to remove this clip from the history?"), 
	              "gtk-yes", NULL))
        {
            DBG ("Removed the selected clip from the History");
	    
	    dtext = gtk_clipboard_wait_for_text (defaultClip);
            if (dtext && !strcmp(dtext, action->clip->text))
	    {
                gtk_clipboard_clear (defaultClip);
		gtk_clipboard_set_text (defaultClip, "", -1);
	    }
	    g_free (dtext);
	    
	    ptext = gtk_clipboard_wait_for_text (primaryClip);
            if (ptext && !strcmp(ptext, action->clip->text))
	    {
                gtk_clipboard_clear (primaryClip);
		gtk_clipboard_set_text (primaryClip, "", -1);
	    }
            g_free (ptext);
	    
            g_ptr_array_remove (action->clipman->clips, action->clip);
            clipman_free_clip (action->clip);
        }
    }
    
    g_free (action);
    
    return FALSE;
}

static GtkWidget *
clipman_create_menuitem (ClipmanAction *action,
                         guint          width,
                         guint          number,
                         gboolean       bold)
{
    GtkWidget *mi;
    GtkLabel  *label;
    gchar     *title;

    mi = gtk_menu_item_new_with_label  ("");
    
    if (bold)
        title = g_strdup_printf("<b>%s</b>", action->clip->title);
    else
	title = g_strdup_printf("%s", action->clip->title);
    
    
    if (action->clipman->ItemNumbers)
    {
        if (number < 10)
            title = g_strdup_printf("<tt><span size=\"smaller\">%d. </span></tt> %s", number ,title);
        else
            title = g_strdup_printf("<tt><span size=\"smaller\">%d.</span></tt> %s",  number ,title);
    }

    label = GTK_LABEL(GTK_BIN(mi)->child);
    gtk_label_set_markup (label, title);
    gtk_label_set_single_line_mode (label, TRUE);
    gtk_label_set_ellipsize (label, PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars (label, width);

    return mi;
}

static void
clipman_clicked_separated (GtkWidget     *menu,
                           ClipmanPlugin *clipman)
{
    gchar         *ptext, *dtext;
    guint          i, j;
    ClipmanAction *action = NULL;
    ClipmanClip   *clip;
    GtkWidget     *mi;
    
    /* Default Clips */
    dtext = gtk_clipboard_wait_for_text (defaultClip);
    j = 0;
    
    for (i = clipman->clips->len; i--; )
    {
        clip = g_ptr_array_index (clipman->clips, i);
        
        if (clip->fromtype == DEFAULT)
        {
            j++;
            
            action = g_new0 (ClipmanAction, 1);
            action->clipman = clipman;
            action->clip = clip;
            
            if (dtext != NULL                 && 
                G_LIKELY (clip->text != NULL) && 
                strcmp(clip->text, dtext) == 0)
            {
                mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                              j, TRUE);
		g_free (dtext);
		dtext = NULL;
            }
            else
            {
                mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                              j, FALSE);
            }
            
            g_signal_connect (G_OBJECT(mi), "button_release_event",
                    G_CALLBACK(clipman_item_clicked), action);
            
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        }
    }
    
    g_free (dtext);

    if (j == 0)
    {
        mi = gtk_menu_item_new_with_label (_("< Default History Empty >"));
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }
    
    /* Primairy Clips */
    mi = gtk_separator_menu_item_new ();
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        
    ptext = gtk_clipboard_wait_for_text (primaryClip);
    j = 0;
       
    for (i = clipman->clips->len; i--; )
    {
        clip = g_ptr_array_index (clipman->clips, i);
         
        if (clip->fromtype == PRIMARY)
        {
            j++;
                
            action = g_new0 (ClipmanAction, 1);
            action->clipman = clipman;
            action->clip = clip;
                
            if (ptext != NULL                 && 
                G_LIKELY (clip->text != NULL) && 
                strcmp(clip->text, ptext) == 0)
            {
                mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                              j, TRUE);
                g_free (ptext);
		ptext = NULL;
            }
            else
            {
                mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                              j, FALSE);
            }
                
            g_signal_connect (G_OBJECT(mi), "button_release_event",
                    G_CALLBACK(clipman_item_clicked), action);
               
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        }
    }
    
    g_free (ptext);
        
    if (j == 0)
    {
        mi = gtk_menu_item_new_with_label (_("< Selection History Empty >"));
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }
    
    mi = gtk_separator_menu_item_new ();
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    mi = gtk_menu_item_new_with_label (_("Clear History"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    g_signal_connect (G_OBJECT (mi), "button_release_event",
        G_CALLBACK (clipman_clear), clipman);
}

static void
clipman_clicked_not_separated (GtkWidget     *menu,
                               ClipmanPlugin *clipman)
{
    gchar         *ptext, *dtext;
    guint          i;
    ClipmanAction *action = NULL;
    ClipmanClip   *clip;
    GtkWidget     *mi;
    
    ptext = gtk_clipboard_wait_for_text (primaryClip);
    dtext = gtk_clipboard_wait_for_text (defaultClip);
    
    for (i = clipman->clips->len; i--;)
    {
        clip = g_ptr_array_index (clipman->clips, i);
        
        action = g_new0 (ClipmanAction, 1);
        action->clipman = clipman;
        action->clip = clip;
        
        if (dtext != NULL                 && 
            G_LIKELY (clip->text != NULL) && 
            strcmp(clip->text, dtext) == 0)
        {
            mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                          clipman->clips->len-i, TRUE);
            g_free (dtext);
	    dtext = NULL;
        }
        else if (ptext != NULL                 && 
                 G_LIKELY (clip->text != NULL) && 
                 strcmp(clip->text, ptext) == 0)
        {
            mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                          clipman->clips->len-i, TRUE);
            g_free (ptext);
	    ptext = NULL;
        }
        else
        {
            mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                          clipman->clips->len-i, FALSE);
        }
        
        g_signal_connect (G_OBJECT(mi), "button_release_event",
                G_CALLBACK(clipman_item_clicked), action);
        
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }
    
    g_free (ptext);
    g_free (dtext);
    
    mi = gtk_separator_menu_item_new ();
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    mi = gtk_menu_item_new_with_label (_("Clear History"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        
    g_signal_connect (G_OBJECT (mi), "button_release_event",
        G_CALLBACK (clipman_clear), clipman);
}

static void
clipman_position_menu (GtkMenu       *menu,
                       int           *x,
                       int           *y,
                       gboolean      *push_in,
                       ClipmanPlugin *clipman)
{
    GtkRequisition  req;
    GdkScreen      *screen;
    GdkRectangle    geom;
    gint            num;
    
    gtk_widget_size_request (GTK_WIDGET (menu), &req);
    
    gdk_window_get_origin (GTK_WIDGET (clipman->plugin)->window, x, y);
    
    switch (xfce_panel_plugin_get_screen_position(clipman->plugin))
    {
        case XFCE_SCREEN_POSITION_SW_H:
        case XFCE_SCREEN_POSITION_S:
        case XFCE_SCREEN_POSITION_SE_H:
            DBG("Bottom");
            *y -= req.height;
            break;
        
        case XFCE_SCREEN_POSITION_NW_H:
        case XFCE_SCREEN_POSITION_N:
        case XFCE_SCREEN_POSITION_NE_H:
            DBG("Top");
            *y += clipman->button->allocation.height;
            break;
        
        case XFCE_SCREEN_POSITION_NW_V:
        case XFCE_SCREEN_POSITION_W:
        case XFCE_SCREEN_POSITION_SW_V:
            DBG("Left");
            *x += clipman->button->allocation.width;
            *y += clipman->button->allocation.height - req.height;
            break;
        
        case XFCE_SCREEN_POSITION_NE_V:
        case XFCE_SCREEN_POSITION_E:
        case XFCE_SCREEN_POSITION_SE_V:
            DBG("Right");
            *x -= req.width;
            *y += clipman->button->allocation.height - req.height;
            break;
        
        case XFCE_SCREEN_POSITION_FLOATING_H:
        case XFCE_SCREEN_POSITION_FLOATING_V:
        case XFCE_SCREEN_POSITION_NONE:
            DBG("Floating");
            gdk_display_get_pointer(gtk_widget_get_display(GTK_WIDGET(clipman->plugin)),
                NULL, x, y, NULL);
    }

    screen = gtk_widget_get_screen (clipman->button);
    num = gdk_screen_get_monitor_at_window (screen, clipman->button->window);
    gdk_screen_get_monitor_geometry (screen, num, &geom);
    
    if (*x > geom.x + geom.width - req.width)
        *x = geom.x + geom.width - req.width;
    if (*x < geom.x)
        *x = geom.x;
    
    if (*y > geom.y + geom.height - req.height)
        *y = geom.y + geom.height - req.height;
    if (*y < geom.y)
        *y = geom.y;
}

static gboolean
clipman_clicked (GtkWidget      *button,
                 GdkEventButton *ev,
                 ClipmanPlugin  *clipman)
{
    GtkWidget *mi;
    GtkWidget *menu;
    gchar     *title;
    
    if (ev->button != 1)
	return FALSE;
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (clipman->button), TRUE);
    
    menu = gtk_menu_new ();

    title = g_strdup_printf("<span weight=\"bold\">%s</span>", _("Clipman History"));
    mi = gtk_menu_item_new_with_label  ("");
    gtk_label_set_markup(GTK_LABEL(GTK_BIN(mi)->child), title);
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    mi = gtk_separator_menu_item_new ();
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    if (clipman->SeparateMenu  && 
        !clipman->IgnoreSelect && 
        G_LIKELY (clipman->clips->len > 0))
    {
        clipman_clicked_separated (menu, clipman);
    }
    else if (G_LIKELY (clipman->clips->len > 0))
    {
        clipman_clicked_not_separated (menu, clipman);
    }
    else
    {
        mi = gtk_menu_item_new_with_label (_("< Clipboard Empty >"));
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }
    
    gtk_widget_show_all (menu);
    
    /* Also destroy the menu items when nothing is clicked */
    g_signal_connect (G_OBJECT(menu), "deactivate",
        G_CALLBACK(clipman_destroy_menu), clipman);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 
                        (GtkMenuPositionFunc) clipman_position_menu, 
                        clipman, 0, 
                        gtk_get_current_event_time ());
    
    return TRUE;
}

static void
clipman_restore_empty (ClipmanPlugin *clipman,
                       ClipboardType  type)
{
    guint        i;
    ClipmanClip *clip;

    if (clipman->Behaviour == STRICTLY &&
        G_LIKELY (clipman->clips->len > 0))
    {
        /* Walk through the array till a clip of it's own 
         * type is found, then past it in the clipboard */
        for (i = clipman->clips->len; i--;)
        {
            clip = g_ptr_array_index (clipman->clips, i);
            
            if (clip->fromtype == type)
            {
                switch (type)
                {
                    case PRIMARY:
                        gtk_clipboard_set_text(primaryClip, clip->text, -1);
                        break;
                    case DEFAULT:
                        gtk_clipboard_set_text(defaultClip, clip->text, -1);
                        break;
                }
                
                DBG("Clipboard restored with a clip from same type");
                
                break;
            }
        }
    }
    else if (clipman->clips->len > 0)
    {
        /* Grap the latest clip and paste it in the clipboard */
        clip = g_ptr_array_index (clipman->clips, (clipman->clips->len-1));
        
        switch (type)
        {
            case PRIMARY:
                gtk_clipboard_set_text(primaryClip, clip->text, -1);
                break;
            case DEFAULT:
                gtk_clipboard_set_text(defaultClip, clip->text, -1);
                break;
        }
        
        DBG("Last clip added");
    }    
}

static gboolean
clipman_check (ClipmanPlugin *clipman)
{
    gchar           *ptext = NULL, *dtext;
    GdkModifierType  state;
    
    /* We ignore the selection clipboard entirely if you've activated this in the options dialog */
    if (!clipman->IgnoreSelect)
    {
	/* Get mouse button information */
	gdk_window_get_pointer(NULL, NULL, NULL, &state);
	
        ptext = gtk_clipboard_wait_for_text (primaryClip);
        
        if (clipman->PreventEmpty && ptext == NULL)
        {
            clipman_restore_empty (clipman, PRIMARY);
        }
        else if (ptext != NULL               &&
                 !(state & GDK_BUTTON1_MASK) && 
                 !clipman_exists (clipman, ptext, PRIMARY))
        {
            DBG("Item added from primary clipboard");
            clipman_add_clip (clipman, ptext, PRIMARY);
            clipman_check_array_len (clipman);
        }
        
        g_free (ptext);
    }
    
    dtext = gtk_clipboard_wait_for_text (defaultClip);

    /* Check default clipboard */
    if (clipman->PreventEmpty && dtext == NULL)
    {
        clipman_restore_empty (clipman, DEFAULT);
    }
    else if (G_LIKELY (dtext != NULL) &&
	     !clipman_exists (clipman, dtext, DEFAULT))
    {
        DBG("Item added from default clipboard");
        clipman_add_clip (clipman, dtext, DEFAULT);
        clipman_check_array_len (clipman);
    }

    g_free (dtext);
    
    return TRUE;
}

static void
clipman_reset_timeout (ClipmanPlugin *clipman)
{
    if (!(clipman->killTimeout))
    {
        if (clipman->TimeoutId != 0)
            g_source_remove (clipman->TimeoutId);
        
        clipman->TimeoutId = g_timeout_add_full (G_PRIORITY_LOW,
                                                 TIMER_INTERVAL,
                                                 (GSourceFunc) clipman_check,
                                                 clipman,
                                                 (GDestroyNotify) clipman_reset_timeout);
    }
}

void
clipman_save (XfcePanelPlugin *plugin,
              ClipmanPlugin   *clipman)
{
    XfceRc      *rc;
    gchar       *file;
    guint        i;
    gchar        name[13];
    ClipmanClip *clip;
    
    DBG("Saving clipman settings");

    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, "xfce4/panel/clipman.rc", TRUE);

    if (G_UNLIKELY (!file))
        return;
    
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);
    
    /* Save the preferences */
    xfce_rc_set_group (rc, "Properties");
    
    xfce_rc_write_bool_entry (rc, "ExitSave",     clipman->ExitSave);
    xfce_rc_write_bool_entry (rc, "IgnoreSelect", clipman->IgnoreSelect);
    xfce_rc_write_bool_entry (rc, "PreventEmpty", clipman->PreventEmpty);
    
    switch (clipman->Behaviour)
    {
        case NORMAL:
            xfce_rc_write_int_entry (rc, "Behaviour", 1);
            break;
        case STRICTLY:
            xfce_rc_write_int_entry (rc, "Behaviour", 2);
            break;
    }
    
    xfce_rc_write_bool_entry (rc, "ItemNumbers",  clipman->ItemNumbers);
    xfce_rc_write_bool_entry (rc, "SeparateMenu", clipman->SeparateMenu);
    
    xfce_rc_write_int_entry (rc, "HistoryItems",   clipman->HistoryItems);
    xfce_rc_write_int_entry (rc, "MenuCharacters", clipman->MenuCharacters);
    
    /* Remove old content and create a new one */
    xfce_rc_delete_group (rc, "Clips", TRUE );
    
    if (clipman->ExitSave && 
        clipman->clips->len > 0
       )
    {
        DBG("Saving the clipboard history");
        
        xfce_rc_set_group (rc, "Clips");
        xfce_rc_write_int_entry (rc, "ClipsLen", clipman->clips->len);
        
        for (i = 0; i < clipman->clips->len; ++i)
        {
            clip = g_ptr_array_index (clipman->clips, i);
            
            g_snprintf (name, 13, "clip_%d_text", i);
            xfce_rc_write_entry (rc, name, clip->text);
            
            g_snprintf (name, 13, "clip_%d_from", i);
            if (clip->fromtype == PRIMARY)
                xfce_rc_write_int_entry (rc, name, 0);
            else
                xfce_rc_write_int_entry (rc, name, 1);
        }
    }

    xfce_rc_close (rc);
}

static void
clipman_read (ClipmanPlugin *clipman)
{
    XfceRc      *rc;
    gchar       *file, *value;
    guint        type, i, clipslen;
    gchar        name[13];
    
    /* Because Clipman is unique, we use 1 config file */
    /*
    file = xfce_panel_plugin_save_location (clipman->plugin, FALSE);
    DBG("Read from file: %s", file);
    */
    
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, "xfce4/panel/clipman.rc", TRUE);

    if (G_UNLIKELY (!file))
        return;
    
    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);
    
    xfce_rc_set_group (rc, "Properties");
    
    clipman->ExitSave         = xfce_rc_read_bool_entry (rc, "ExitSave",     DEFEXITSAVE);
    clipman->IgnoreSelect     = xfce_rc_read_bool_entry (rc, "IgnoreSelect", DEFIGNORESELECT);
    clipman->PreventEmpty     = xfce_rc_read_bool_entry (rc, "PreventEmpty", DEFPREVENTEMPTY);
    
    switch (xfce_rc_read_int_entry (rc, "Behaviour", DEFBEHAVIOUR))
    {
        case 1:
            clipman->Behaviour = NORMAL;
            DBG ("Behaviour = NORMAL");
            break;
        case 2:
            clipman->Behaviour = STRICTLY;
            DBG ("Behaviour = STRICTLY");
            break;
    }
    
    clipman->ItemNumbers      = xfce_rc_read_bool_entry (rc, "ItemNumbers",    DEFITEMNUMBERS);
    clipman->SeparateMenu     = xfce_rc_read_bool_entry (rc, "SeparateMenu",   DEFSEPMENU);
    
    clipman->HistoryItems     = xfce_rc_read_int_entry  (rc, "HistoryItems",   DEFHISTORY);
    clipman->MenuCharacters   = xfce_rc_read_int_entry  (rc, "MenuCharacters", DEFCHARS);
    
    if (clipman->HistoryItems > MAXHISTORY)
        clipman->HistoryItems = MAXHISTORY;
    if (clipman->HistoryItems < MINHISTORY)
        clipman->HistoryItems = MINHISTORY;
    
    if (clipman->MenuCharacters > MAXCHARS)
        clipman->MenuCharacters = MAXCHARS;
    if (clipman->MenuCharacters < MINCHARS)
        clipman->MenuCharacters = MINCHARS;
    
    xfce_rc_set_group (rc, "Clips");
    clipslen = xfce_rc_read_int_entry (rc, "ClipsLen", 0);
    
    if (clipslen > MAXHISTORY)
        clipslen = MAXHISTORY;
    
    if (clipman->ExitSave && 
        clipslen > 0
       )
    {
        DBG("Restoring the clipboard");
        
        for (i = 0; i < clipslen; ++i)
        {
            g_snprintf (name, 13, "clip_%d_text", i);
            value = g_strdup (xfce_rc_read_entry (rc, name, ""));
            
            g_snprintf (name, 13, "clip_%d_from", i);
            type = xfce_rc_read_int_entry (rc, name, 0);
            
            if (type == 0)
                clipman_add_clip (clipman, value, PRIMARY);
            else
                clipman_add_clip (clipman, value, DEFAULT);
	    
	    g_free (value);
        }
    }
    
    xfce_rc_close (rc);
}

static ClipmanPlugin *
clipman_new (XfcePanelPlugin *plugin)
{
    ClipmanPlugin *clipman;
    clipman = g_new0 (ClipmanPlugin, 1);
    
    clipman->clips = g_ptr_array_new ();
    clipman->plugin = plugin;
    
    clipman->tooltip = gtk_tooltips_new ();
    g_object_ref (G_OBJECT (clipman->tooltip));
    
    /* Load Settings */
    clipman_read (clipman);
    
    /* Create panel widgets */
    clipman->button = xfce_create_panel_toggle_button ();
    gtk_widget_show (clipman->button);
    
    clipman->icon = gtk_image_new ();
    gtk_widget_show (clipman->icon);
    gtk_container_add (GTK_CONTAINER (clipman->button), clipman->icon);
        
    gtk_tooltips_set_tip (GTK_TOOLTIPS(clipman->tooltip),
                          clipman->button, _("Clipboard Manager"),
                          NULL);
    
    g_signal_connect(clipman->button, "button_press_event",
            G_CALLBACK(clipman_clicked), clipman);
    
    /* Start the clipman_check function */
    clipman->TimeoutId = g_timeout_add_full(G_PRIORITY_LOW,
                                            TIMER_INTERVAL,
                                            (GSourceFunc) clipman_check,
                                            clipman,
                                            (GDestroyNotify) clipman_reset_timeout);
    
    /* Connect to the clipboards */
    defaultClip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    primaryClip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
            
    return clipman;
}

static void
clipman_free (XfcePanelPlugin *plugin,
              ClipmanPlugin   *clipman)
{
    guint        i;
    ClipmanClip *clip;
    GtkWidget   *dialog;
    
    /* Valgrind notes:
       - primaryClip and defaultClip should be cleared, but gtk
         takes care about this
    */
    
    /* Free the clipboards */
    gtk_clipboard_clear (primaryClip);
    gtk_clipboard_clear (defaultClip);

    /* Destroy the setting dialog, if this open */
    dialog = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dialog)
        gtk_widget_destroy (dialog);

    /* Stop the check loop */
    clipman->killTimeout = TRUE;
    if (clipman->TimeoutId != 0)
    {
        g_source_remove(clipman->TimeoutId);
        clipman->TimeoutId = 0;
    }
    
    /* Remove clipboard items */
    for (i = clipman->clips->len; i--;)
    {
        clip = g_ptr_array_index (clipman->clips, i);
        g_ptr_array_remove_fast (clipman->clips, clip);
        clipman_free_clip (clip);
    }
    g_ptr_array_free (clipman->clips, TRUE);

    gtk_tooltips_set_tip (clipman->tooltip, clipman->button, NULL, NULL);
    g_object_unref (G_OBJECT (clipman->tooltip));
    
    gtk_widget_destroy (clipman->icon);
    gtk_widget_destroy (clipman->button);
    
    clipman->plugin = NULL;
    
    DBG ("Plugin Freed");
    
    g_free (clipman);
}

static gboolean
clipman_set_size (XfcePanelPlugin *plugin,
                  gint             wsize,
                  ClipmanPlugin   *clipman)
{
    GdkPixbuf *pb;
    gint       size;
    
    gtk_widget_set_size_request (clipman->button, wsize, wsize);
    
    size = wsize - 2 - (2 * MAX (clipman->button->style->xthickness,
                                 clipman->button->style->ythickness));

    DBG("Set clipman button size to %dpx, load icon at %dpx", wsize, size);
    
    pb = xfce_themed_icon_load ("gtk-paste", size);
    gtk_image_set_from_pixbuf (GTK_IMAGE (clipman->icon), pb);
    g_object_unref (G_OBJECT (pb));
 
    return TRUE;
}

static void 
clipman_construct (XfcePanelPlugin *plugin)
{
    ClipmanPlugin *clipman;

    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
    
    clipman = clipman_new (plugin);
    
    gtk_container_add (GTK_CONTAINER (plugin), clipman->button);
    
    xfce_panel_plugin_add_action_widget (plugin, clipman->button);
    
    g_signal_connect (plugin, "free-data", 
        G_CALLBACK (clipman_free), clipman);
    
    g_signal_connect (plugin, "save", 
        G_CALLBACK (clipman_save), clipman);
    
    g_signal_connect (plugin, "size-changed",
        G_CALLBACK (clipman_set_size), clipman);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
        G_CALLBACK (clipman_configure), clipman);
}
