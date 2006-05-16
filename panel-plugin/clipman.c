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

#include "clipman.h"
#include "clipman-dialogs.h"

static void clipman_construct (XfcePanelPlugin *plugin);

/* Register Plugin */
XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (clipman_construct);

void
clipman_free_clip (ClipmanClip *clip)
{
    DBG("...");
    
    g_free (clip->text);
    g_free (clip->title);
    
    g_free (clip);
}

void
clipman_replace_text (gchar *o_string, gchar *n_string)
{
    if ( gtk_clipboard_wait_for_text (defaultClip) &&
         !strcmp(gtk_clipboard_wait_for_text (defaultClip), o_string)
       )
    {
        gtk_clipboard_set_text(defaultClip, n_string, -1);
    }
        
    if ( gtk_clipboard_wait_for_text (primaryClip) &&
         !strcmp(gtk_clipboard_wait_for_text (primaryClip), o_string)
       )
    {
        gtk_clipboard_set_text(primaryClip, n_string, -1);
    }
}

static void
clipman_destroy_menu (GtkWidget *menu, ClipmanPlugin *clipman)
{
    DBG("...");
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (clipman->button), FALSE);
    
    gtk_widget_destroy (menu);
}

static gboolean
clipman_clear (GtkWidget      *mi,
               GdkEventButton *ev,
               ClipmanPlugin  *clipman)
{
    if (xfce_confirm (_("Are you sure you want to clear the history?"), 
	              "gtk-yes", 
                      NULL))
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
    
    return FALSE;
}

void
clipman_check_array_len (ClipmanPlugin *clipman)
{
     ClipmanClip *clip;
    
    if (clipman->block)
        return;
    
    while (clipman->clips->len > clipman->HistoryItems)
    {
        clip = g_ptr_array_index (clipman->clips, 0);
        g_ptr_array_remove (clipman->clips, clip);
        clipman_free_clip (clip);
        
        DBG("A clip hase been removed");
    }
}

gchar *
clipman_create_title (gchar *txt,
                      gint chars)
{
    guint i;
    
    txt = g_strndup(txt, chars);

    i = 0;
    while (txt[i] != '\0')
    {
        if (txt[i] == '\n' || txt[i] == '\r' || txt[i] == '\t')
            txt[i] = ' ';
        i++;
    }

    if (!g_utf8_validate(txt, -1, NULL))
    {
        txt = g_locale_to_utf8(txt, -1, NULL, NULL, NULL);
        DBG("Title was not UTF-8 complaint");
    }
    
    txt = g_markup_printf_escaped ("%s", txt);
    txt = g_strstrip(txt);
    return txt;
}

void
clipman_remove_selection_clips (ClipmanPlugin *clipman)
{
    ClipmanClip *clip;
    gint         i;

    if (!clipman->IgnoreSelect)
        return;
    
    DBG ("Cleaning up all selection clips");
    
    for (i = 0; i < clipman->clips->len; i++)
    {
        clip = g_ptr_array_index (clipman->clips, i);
    
        if (clip->fromtype == PRIMARY)
        {
             g_ptr_array_remove (clipman->clips, clip);
             clipman_free_clip (clip);
        }
    }
}

void
clipman_regenerate_titles (ClipmanPlugin *clipman,
                           gint           MenuCharacters)
{
    ClipmanClip *clip;
    gint         i;

    DBG ("Regenerating titles");
    
    for (i = 0; i < clipman->clips->len; i++)
    {
        clip = g_ptr_array_index (clipman->clips, i);
        
        clip->title = clipman_create_title (clip->text, MenuCharacters);
    }
}

static void
clipman_add_clip (ClipmanPlugin *clipman, gchar *txt, ClipboardType type)
{
    ClipmanClip *new_clip;

    DBG("...");
    
    if (txt != "" &&
        !clipman->block
       )
    {
        new_clip = g_new0 (ClipmanClip, 1);
        
        new_clip->text     = g_strdup (txt);
        new_clip->title    = clipman_create_title (txt, clipman->MenuCharacters);
        new_clip->fromtype = type;
        
        g_ptr_array_add (clipman->clips, new_clip);
        
        DBG("Clips %d/%d", clipman->clips->len, clipman->HistoryItems);
    }
}

static gboolean
clipman_exists (ClipmanPlugin *clipman,
                gchar         *txt,
                ClipboardType  type)
{
    gint         i;
    ClipmanClip *clip;
    
    /* Walk through the array backwards, because
     * if the text exists, this will probably be the newest */

    for (i = (clipman->clips->len - 1); i >= 0; i--)
    {
        clip = g_ptr_array_index (clipman->clips, i);
        
        if (G_LIKELY ((clip->text != NULL) && 
                      (strcmp(clip->text, txt) == 0))
           )
        {
            /* When clipboard restoring is allowed a clip will be moved from
             * the pimairy clip to the default clip when it exists */
            if (type == DEFAULT && 
                clip->fromtype == PRIMARY && 
                clipman->Behaviour == NORMAL
               )
                clip->fromtype = DEFAULT;
            
            /* If strictly; only return true when the clipboard types are the same */
            if (clipman->Behaviour == STRICTLY &&
                type == clip->fromtype
               )
                return TRUE;
            
            else if (clipman->Behaviour != STRICTLY )
                return TRUE;
        }
    }
    
    return FALSE;
}

static gboolean
clipman_item_clicked (GtkWidget      *mi,
                      GdkEventButton *ev,
                      ClipmanAction  *action)
{
    if (ev->button == 1 && action->clipman->Behaviour == STRICTLY)
    {
        DBG("Clip copied to his own clipboard (STRICTLY)");
        
        if (action->clip->fromtype == DEFAULT)
        {
            gtk_clipboard_set_text(defaultClip, action->clip->text, -1);
        }
        
        if (action->clip->fromtype == PRIMARY)
        {
            gtk_clipboard_set_text(primaryClip, action->clip->text, -1);
        }
    }
    else if (ev->button == 1)
    {
        DBG("Clip copied to both clipboards");
        
        gtk_clipboard_set_text(defaultClip, action->clip->text, -1);
        gtk_clipboard_set_text(primaryClip, action->clip->text, -1);
    }
    else if (ev->button == 3)
    {
        clipman_question (action);
    }
    
    return FALSE;
}

static GtkWidget *
clipman_create_menuitem (ClipmanAction *action,
                         gint           number,
                         gboolean       bold)
{
    GtkWidget *mi;
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
    
    gtk_label_set_markup (GTK_LABEL(GTK_BIN(mi)->child), title);
    
    return mi;
}

static void
clipman_clicked_separated (GtkWidget     *menu,
                           ClipmanPlugin *clipman)
{
    gchar         *priClip, *defClip;
    gint           i, j;
    ClipmanAction *action = NULL;
    ClipmanClip   *clip;
    GtkWidget     *mi;
    
    defClip = gtk_clipboard_wait_for_text (defaultClip);
    j = 0;
    
    for (i = (clipman->clips->len - 1); i >= 0; i--)
    {
        clip = g_ptr_array_index (clipman->clips, i);
        
        if (clip->fromtype == DEFAULT)
        {
            j++;
            
            action = g_new0 (ClipmanAction, 1);
            action->clipman = clipman;
            action->clip = clip;
            
            if ((defClip != NULL) && 
                (clip->text != NULL) && 
                (strcmp(clip->text, defClip) == 0)
               )
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
        mi = gtk_menu_item_new_with_label (_("< Default History Empty >"));
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }
    
    if (!clipman->IgnoreSelect)
    {
        mi = gtk_separator_menu_item_new ();
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        
        priClip = gtk_clipboard_wait_for_text (primaryClip);
        j = 0;
        
        for (i = (clipman->clips->len - 1); i >= 0; i--)
        {
            clip = g_ptr_array_index (clipman->clips, i);
            
            if (clip->fromtype == PRIMARY)
            {
                j++;
                
                action = g_new0 (ClipmanAction, 1);
                action->clipman = clipman;
                action->clip = clip;
                
                if ((priClip != NULL) && 
                    (clip->text != NULL) && 
                    (strcmp(clip->text, priClip) == 0)
                   )
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
        g_free (priClip);
        
        if (j == 0)
        {
            mi = gtk_menu_item_new_with_label (_("< Selection History Empty >"));
            gtk_widget_set_sensitive (mi, FALSE);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        }
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
clipman_clicked_not_separated (GtkWidget *menu, ClipmanPlugin *clipman)
{
    gchar         *priClip, *defClip;
    gint           i;
    ClipmanAction *action = NULL;
    ClipmanClip   *clip;
    GtkWidget     *mi;
    
    priClip = gtk_clipboard_wait_for_text (primaryClip);
    defClip = gtk_clipboard_wait_for_text (defaultClip);
    
    for (i = (clipman->clips->len - 1); i >= 0; i--)
    {
        clip = g_ptr_array_index (clipman->clips, i);
        
        action = g_new0 (ClipmanAction, 1);
        action->clipman = clipman;
        action->clip = clip;
        
        if ((defClip != NULL) && 
            (clip->text != NULL) && 
            (strcmp(clip->text, defClip) == 0)
           )
        {
            mi = clipman_create_menuitem (action, clipman->clips->len-i, TRUE);
            priClip = NULL;
        }
        else if ((priClip != NULL) && 
                 (clip->text != NULL) && 
                 (strcmp(clip->text, priClip) == 0)
                )
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
    if (ev->button != 1)
	return FALSE;
    
    GtkWidget *mi;
    GtkWidget *menu;
    gchar     *title;
    
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
    
    if (clipman->SeparateMenu && 
        !clipman->IgnoreSelect && 
        clipman->clips->len > 0
       )
    {
        clipman_clicked_separated (menu, clipman);
    }
    else if (clipman->clips->len > 0)
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
    gint         i;
    ClipmanClip *clip;

    if (clipman->Behaviour == STRICTLY &&
        clipman->clips->len > 0
       )
    {
        /* Walk through the array till a clip of it's own 
         * type is found, then past it in the clipboard */
        for (i = (clipman->clips->len - 1); i >= 0; i--)
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
    gchar           *txt;
    GdkModifierType  state;
  
    /* Do nothing when the clipboard is blocked */
    if (G_UNLIKELY(clipman->block))
        return TRUE;
    
    /* We ignore the selection clipboard entirely if you've activated this in the options dialog */
    if (!clipman->IgnoreSelect)
    {
	/* Get mouse button information */
	gdk_window_get_pointer(NULL, NULL, NULL, &state);
	
        txt = gtk_clipboard_wait_for_text (primaryClip);
        
        if (clipman->PreventEmpty && G_UNLIKELY (txt == NULL))
        {
            clipman_restore_empty (clipman, PRIMARY);
        }
        else if (txt != NULL &&
                 !(state & GDK_BUTTON1_MASK) && 
                 !clipman_exists(clipman, txt, PRIMARY)
            )
        {
            DBG("Item added from primary clipboard");
            clipman_add_clip (clipman, txt, PRIMARY);
            clipman_check_array_len (clipman);
        }
        
        g_free (txt);
        txt = NULL;
    }
    
    txt = gtk_clipboard_wait_for_text (defaultClip);

    /* Check default clipboard */
    if (clipman->PreventEmpty && 
        G_UNLIKELY (txt == NULL)
       )
    {
        clipman_restore_empty (clipman, DEFAULT);
    }
    else if (G_LIKELY (txt != NULL))
    {
        if (!clipman_exists(clipman, txt, DEFAULT))
        {
            DBG("Item added from default clipboard");
            clipman_add_clip (clipman, txt, DEFAULT);
            clipman_check_array_len (clipman);
        }
    }

    g_free (txt);
    
    return TRUE;
}

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

void
clipman_save (XfcePanelPlugin *plugin,
              ClipmanPlugin   *clipman)
{
    XfceRc      *rc;
    gchar       *file;
    gint         i;
    gchar        name[13];
    ClipmanClip *clip;
    
    DBG("...");

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
    gchar       *file;
    gint         type, i, clipslen;
    gchar        name[13];
    const gchar *value;
    
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
            value = xfce_rc_read_entry (rc, name, "");
            
            g_snprintf (name, 13, "clip_%d_from", i);
            type = xfce_rc_read_int_entry (rc, name, 0);
            
            if (type == 0)
                clipman_add_clip (clipman, (gchar *)value, PRIMARY);
            else
                clipman_add_clip (clipman, (gchar *)value, DEFAULT);
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
    
    clipman_read (clipman);
    
    clipman->button = gtk_toggle_button_new ();
        gtk_widget_show (clipman->button);
        gtk_button_set_relief (GTK_BUTTON (clipman->button), GTK_RELIEF_NONE);
        gtk_button_set_focus_on_click (GTK_BUTTON (clipman->button), FALSE);
        
    gtk_tooltips_set_tip (GTK_TOOLTIPS(clipman->tooltip),
                clipman->button, _("Clipboard Manager"),
                NULL
                 );
    
    g_signal_connect(clipman->button, "button_press_event",
            G_CALLBACK(clipman_clicked), clipman);
    
    /* Start the clipman_check function */
    clipman->timeId = g_timeout_add_full(G_PRIORITY_LOW, TIMER_INTERVAL,
        (GSourceFunc)clipman_check, clipman, (GDestroyNotify)clipman_reset_timer);
            
    /* Connect to the clipboards */
    defaultClip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    primaryClip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
            
    return clipman;
}

static void
clipman_free (XfcePanelPlugin *plugin,
              ClipmanPlugin   *clipman)
{
    gint         i;
    ClipmanClip *clip;
    GtkWidget   *dialog;

    dialog = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dialog)
        gtk_widget_destroy (dialog);

    /* Stop the check loop */
    clipman->killTimeout = TRUE;
    if (clipman->timeId != 0)
        g_source_remove(clipman->timeId);
    
    /* Remove clipboard items */
    for (i = 0; i < clipman->clips->len; ++i)
    {
        clip = g_ptr_array_index (clipman->clips, i);
        clipman_free_clip (clip);
    }
    g_ptr_array_free (clipman->clips, TRUE);

    gtk_widget_destroy (clipman->icon);
    gtk_widget_destroy (clipman->button);
    
    g_free (clipman->tooltip);
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
    gint       newsize;
    
    DBG("...");

    newsize = wsize - 10;
    if (newsize < 1)
        newsize = 1;
    
    gtk_widget_set_size_request (clipman->button, wsize, wsize);
    
    if (clipman->icon)
        gtk_widget_destroy (clipman->icon);
    
    pb = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), "gtk-paste", newsize , 0, NULL);
    clipman->icon = gtk_image_new_from_pixbuf (pb);
    gtk_widget_show (clipman->icon);
    gtk_container_add (GTK_CONTAINER (clipman->button), clipman->icon);
    
    g_object_unref (pb);

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
