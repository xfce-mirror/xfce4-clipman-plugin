/*  $Id$
 *
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *                2008-2009 Mike Massonnet <mmassonnet@xfce.org>
 *                2008-2009 David Collins <david.8.collins@gmail.com>
 *
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

#include <exo/exo.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>

#include "clipman.h"
#include "clipman-dialogs.h"

/* The clipboards */
static GtkClipboard *primaryClip;
static GtkClipboard *defaultClip;

/* For event-driven clipboard_change() function */
gboolean MouseSelecting=FALSE;
gboolean ShiftSelecting=FALSE;
gboolean IgnoreSignal=FALSE;

/* Register the plugin */
static void
clipman_construct (XfcePanelPlugin *plugin);

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL (clipman_construct);

static void
clipman_free_clip (ClipmanClip *clip)
{
    if (clip->datatype == RAWTEXT) {
      g_free (clip->data);
    } else if (clip->datatype == IMAGE) {
      g_object_unref(clip->data);
    }
    g_free (clip->title);

    panel_slice_free (ClipmanClip, clip);

    DBG ("Clip successfully freed");
}

static void
clipman_set_data (ClipmanClip *clip, GtkClipboard *clipboard)
{
    DBG ("Clipman_set_data ..");
    if (clip->datatype == RAWTEXT) {
      gtk_clipboard_set_text (clipboard, clip->data, -1);
    } else if (clip->datatype == IMAGE) {
      gtk_clipboard_set_image (clipboard, (GdkPixbuf *)clip->data);
    }
    DBG ("Clip data copied to clipboard");
}

static void
clipman_destroy_menu (GtkWidget     *menu,
                      ClipmanPlugin *clipman)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (clipman->button), FALSE);

    gtk_widget_destroy (menu);

    DBG ("Menu Destroyed");
}

/* Clear list of items AND both clipboards */
static gboolean
clipman_clear (GtkWidget *mi, GdkEventButton *ev, ClipmanPlugin *clipman) {

    ClipmanClip *clip;

    if (xfce_confirm (_("Are you sure you want to clear the history?"),
	                                                      "gtk-yes", NULL)) {
	              
      gtk_clipboard_clear (primaryClip);
	    gtk_clipboard_clear (defaultClip);
      clipman->DefaultIndex=-1;
      clipman->PrimaryIndex=-1;

      while (clipman->clips->len > 0)
      {
        clip = g_ptr_array_index (clipman->clips, 0);
        g_ptr_array_remove (clipman->clips, clip);
        clipman_free_clip (clip);
      }
    }
    return FALSE;
}

/* Remove the oldest items - these are at the start of the list */
void
clipman_array_remove_oldest (ClipmanPlugin *clipman)
{
    ClipmanClip *clip;
    gint i;
    
    while (clipman->clips->len > clipman->HistoryItems) {

      // Leave items in list if they are active
      for (i=0; i<2; ++i) {
        if (clipman->DefaultIndex != i && clipman->PrimaryIndex != i) break;
      }

      clip = g_ptr_array_index (clipman->clips, i);
      g_ptr_array_remove (clipman->clips, clip);
      clipman_free_clip (clip);
      /* Adjust indexes to clipboard data */
      if (clipman->DefaultIndex > i) --clipman->DefaultIndex;
      if (clipman->PrimaryIndex > i) --clipman->PrimaryIndex;

      DBG("A clip have been removed");
    }
}

void
clipman_array_remove_image_data (ClipmanPlugin *clipman)
{
    ClipmanClip *clip;
    gint i;
    
    DBG ("Removing image data from array");

    for (i=0; i<clipman->clips->len; ++i) {

      clip = g_ptr_array_index (clipman->clips, i);
      if (clip->datatype == IMAGE) {
        g_ptr_array_remove (clipman->clips, clip);
        clipman_free_clip (clip);
        /* Adjust indexes to clipboard data */
        if (clipman->DefaultIndex == i) clipman->DefaultIndex=-1;
        if (clipman->DefaultIndex > i) --clipman->DefaultIndex;
      }
    }
}

static gchar *
clipman_create_title (gchar *text,
                      gint   length)
{
  gchar                *short_text, *tmp = NULL;
  const gchar          *offset;

  g_return_val_if_fail (G_LIKELY (NULL != text), NULL);

  if (G_UNLIKELY (!g_utf8_validate (text, -1, NULL)))
    return NULL;

  short_text = g_strstrip (g_strdup (text));

  /* Shorten */
  if (g_utf8_strlen (short_text, -1) > length)
    {
      offset = g_utf8_offset_to_pointer (short_text, length);
      tmp = g_strndup (short_text, offset - short_text);
      g_free (short_text);

      short_text = g_strconcat (tmp, "...", NULL); /* Ellipsis */
      g_free (tmp);
    }

  /* Cleanup */
  tmp = g_strdelimit (short_text, "\n\r\t", ' ');
  short_text = g_markup_escape_text (tmp, -1);
  g_free (tmp);

  return short_text;
}

/* Add new item to the end of the list */
static void
clipman_add_clip (ClipmanPlugin *clipman, void *data, ClipboardType cliptype, ClipDataType datatype) {
                  
  ClipmanClip *clip;

  if (G_LIKELY (data != NULL)) {
      // &&  G_LIKELY (strcmp (data, ""))) {
    
    clip = panel_slice_new0 (ClipmanClip);
    
    if (datatype == RAWTEXT) {
      clip->title = clipman_create_title (data, clipman->MenuCharacters);
    } else {
      /* Change this to store a pixbuf preview */
      clip->title = clipman_create_title (CLIPIMAGETITLE, clipman->MenuCharacters);
      clip->preview = exo_gdk_pixbuf_scale_ratio (GDK_PIXBUF (data), 128);
    }

  	/* No valid title could be created, drop it */
	  if (clip->title == NULL) {
	    DBG("A title couldn't be created");
        panel_slice_free (ClipmanClip, clip);
	    return;
	  }

    /* Make a copy of the data and add to the pointer array */
    if (datatype == RAWTEXT) {
      clip->data = g_strdup(data);
      clip->datatype = RAWTEXT;
    } else if (datatype == IMAGE) {
      clip->data = gdk_pixbuf_copy((GdkPixbuf *)data);
      DBG("Made copy of image");
      clip->datatype = IMAGE;
      // Remove any other image data first ..
      clipman_array_remove_image_data(clipman);
    }
    g_ptr_array_add (clipman->clips, clip);
                                                         
    /* Indicate this item is in the clipboard */
    if (cliptype == DEFAULT) {
      clipman->DefaultIndex=clipman->clips->len-1;
    } else if (cliptype == PRIMARY) {
      clipman->PrimaryIndex=clipman->clips->len-1;
    }
    DBG("Added clip %d of %d", clipman->clips->len, clipman->HistoryItems);
  }
}

/* See if the text/image is already in the list.  If so, mark it as the current DEFAULT or PRIMARY clipboard.
   If not found then return FALSE.
   Currently, this will always return FALSE for IMAGE data. */
static gboolean
clipman_exists (ClipmanPlugin *clipman, void *data, ClipboardType cliptype, ClipDataType datatype) {
                
    gint        i;
    ClipmanClip *clip;

    /* Walk through the array backwards, because
     * if the text exists, this will probably be the newest */
    for (i=(gint)clipman->clips->len-1; i>=0; i--) {
    
      clip = g_ptr_array_index (clipman->clips, i);

      if (G_LIKELY(clip->data != NULL)) {
        if (datatype == RAWTEXT && strcmp(clip->data, data) == 0) {
          if (cliptype == PRIMARY) {
            clipman->PrimaryIndex=i;
            DBG("String re-selected");
          } else if (cliptype == DEFAULT) {
            clipman->DefaultIndex=i;
            DBG("Selection Copied");
          }
          return TRUE;
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
    gboolean defaultcleared, primarycleared; 

    /* This function will send a clipboard 'owner-change' signal which we will
     * ignore by setting IgnoreSignal to TRUE. */

    /* Left mouse button - put item in BOTH clipboards */
    if (ev->button == 1) {
      IgnoreSignal=TRUE;
      gtk_clipboard_clear (defaultClip);
      clipman_set_data (action->clip, defaultClip);      
      //gtk_clipboard_set_text (defaultClip, action->clip->data, -1);
      action->clipman->DefaultIndex = action->index;
	    DBG ("Clip copied to default clipboard");

	    if (action->clipman->AddSelect)	{
          IgnoreSignal=TRUE;
	      gtk_clipboard_clear (primaryClip);
        clipman_set_data (action->clip, primaryClip);      
        action->clipman->PrimaryIndex = action->index;
	      DBG ("Clip copied to primary clipboard");
	    }
	    
	  /* Right mouse button - remove item */  
    } else if (ev->button == 3) {

      defaultcleared=FALSE;
      primarycleared=FALSE; 
      
      DBG ("Removed the selected clip from the History");
	    /* If this item is in clipboard, clear the clipbard */
      if (action->clipman->DefaultIndex == action->index) {
        gtk_clipboard_clear (defaultClip);
        defaultcleared=TRUE;
	    } else if (action->clipman->DefaultIndex > action->index) {
	      // index needs adjustment
        --action->clipman->DefaultIndex;
      }
      
	    if (action->clipman->AddSelect)	{
  	    /* If this item is in clipboard, clear the clipbard */
        if (action->clipman->PrimaryIndex == action->index) {
          gtk_clipboard_clear (primaryClip);
          primarycleared=TRUE;
	      } else if (action->clipman->PrimaryIndex > action->index) {
	        // index needs adjustment
          --action->clipman->PrimaryIndex;
    		}
	    }

      /* Remove chosen item from the list */
      g_ptr_array_remove (action->clipman->clips, action->clip);
      clipman_free_clip (action->clip);
      guint len = action->clipman->clips->len;

      /* List is now empty */
      if (len == 0) {
  		  if (defaultcleared) {
   		    gtk_clipboard_set_text (defaultClip, "", -1); // this might not be needed?
   		    action->clipman->DefaultIndex = -1;
   		  }
   		  if (primarycleared) {
   		    gtk_clipboard_set_text (primaryClip, "", -1); // this might not be needed?
     		  action->clipman->PrimaryIndex = -1;
   		  }

      } else {
        /* Get the newest item left in the list */
        ClipmanClip *clip = g_ptr_array_index (action->clipman->clips, len-1);

        /* If Clipboard has been cleared, put in a replacement */
    		if (defaultcleared) {
          clipman_set_data (clip, defaultClip);      
    		  action->clipman->DefaultIndex = len-1;
    		}
    		if (primarycleared) {
          clipman_set_data (clip, primaryClip);      
    		  action->clipman->PrimaryIndex = len-1;
    		}
    	} 
    }

    // Menu disappears ..
    panel_slice_free (ClipmanAction, action);

    return FALSE;
}

static GtkWidget *
clipman_create_menuitem (ClipmanAction *action,
                         guint          width,
                         guint          number,
                         ClipMenuFormat format) {

    GtkWidget *mi;
    gchar     *title, *string_num;

    if (action->clipman->ItemNumbers) {
        if (number < 10)
            string_num = g_strdup_printf("<tt><span size=\"smaller\">%d. </span></tt> ", number);
        else
            string_num = g_strdup_printf("<tt><span size=\"smaller\">%d.</span></tt> ", number);
    } else {
        string_num = g_strdup ("");
    }

    if (format==BOLD)
        title = g_strdup_printf("%s<b>%s</b>", string_num, action->clip->title);
    else if (format==ITALICS)
        title = g_strdup_printf("%s<i>%s</i>", string_num, action->clip->title);
    else
        title = g_strdup_printf("%s%s", string_num, action->clip->title);

    g_free (string_num);

    mi = gtk_menu_item_new_with_label ("");
    gtk_label_set_markup (GTK_LABEL (GTK_BIN (mi)->child), title);
    g_free (title);

    return mi;
}

static GtkWidget *
clipman_create_imagemenuitem (ClipmanAction *action) {

    GtkWidget *mi, *image;

    mi = gtk_menu_item_new ();
    image = gtk_image_new_from_pixbuf (action->clip->preview);
    gtk_container_add (GTK_CONTAINER (mi), image);

    return mi;
}

static void
clipman_build_menu_body (GtkWidget *menu, ClipmanPlugin *clipman) {
                               
    guint          i;
    ClipmanAction *action = NULL;
    ClipmanClip   *clip;
    GtkWidget     *mi;

    for (i=clipman->clips->len; i--;) {
        clip = g_ptr_array_index (clipman->clips, i);

        action = panel_slice_new0 (ClipmanAction);
        action->clipman = clipman;
        action->clip = clip;
        action->index = i;

        if (clip->datatype == RAWTEXT) {
          if (clipman->DefaultIndex == i) {
            mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                          clipman->clips->len-i, BOLD);
          }
          else if (clipman->PrimaryIndex == i) {
            mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                          clipman->clips->len-i, ITALICS);
          } else {
            mi = clipman_create_menuitem (action, clipman->MenuCharacters,
                                          clipman->clips->len-i, PLAIN);
          }
        } else if (clip->datatype == IMAGE) {
          mi = clipman_create_imagemenuitem (action);
        }

        /* TODO action ends in a leak as it gets never freed in the items that
         * are not clicked */
        g_signal_connect (G_OBJECT(mi), "button_release_event",
                G_CALLBACK(clipman_item_clicked), action);
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

/* Show list when the clipman icon is clicked with the left mouse button */
static gboolean
clipman_icon_clicked (GtkWidget      *button,
                 GdkEventButton *ev,
                 ClipmanPlugin  *clipman)
{
    GtkWidget *mi;
    GtkWidget *menu;
    gchar     *title;

    if (ev->button != 1) return FALSE;

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

    if (G_LIKELY (clipman->clips->len > 0)) {
        clipman_build_menu_body (menu, clipman);
    } else {
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

/* Called when a program closes. */
static void
clipman_refill_clipboard (ClipmanPlugin *clipman,
                       ClipboardType  type)
{
    ClipmanClip *clip;

    if (clipman->clips->len > 0) {
    
        if (type == DEFAULT) {
          clip = g_ptr_array_index(clipman->clips, clipman->DefaultIndex);
          clipman_set_data (clip, defaultClip);      
        } else if (type == PRIMARY) {
          clip = g_ptr_array_index(clipman->clips, clipman->PrimaryIndex);
          clipman_set_data (clip, primaryClip);      
        }
        DBG("Active clip restored");
    }
}

/* This function is called when populating the clipboard in these events -
  - user selects data             - add to list
  - user copies data to clipboard - add to list
  - program holding the DEFAULT clipboard closes down - populate DEFAULT clipboard with active list item
  - program holding the PRIMARY clipboard closes down - populate PRIMARY clipboard with active list item
*/
static void clipman_clipboard_changed(GtkClipboard *clipboard, ClipmanPlugin *clipman, GdkOwnerChange reason) {

  gchar *ptext = NULL, *dtext;

  // Information for the programmer
  if (reason == GDK_OWNER_CHANGE_DESTROY) {
    DBG("GDK_OWNER_CHANGE_DESTROY");
  }

  /* DEFAULT clipboard - 'Copied' data */
  if (clipboard == defaultClip) {
       
    // Program holding selection has closed
    // (Mousepad gives GDK_OWNER_CHANGE_CLOSE, Kolorpaint gives GDK_OWNER_CHANGE_DESTROY)
    if (reason == GDK_OWNER_CHANGE_CLOSE || reason == GDK_OWNER_CHANGE_DESTROY) {
      clipman_refill_clipboard (clipman, DEFAULT);
      return;
    }

    // User copied data to clipboard
    if (reason == GDK_OWNER_CHANGE_NEW_OWNER) {

      dtext = gtk_clipboard_wait_for_text (defaultClip);
      if (dtext != NULL) {
        if (!clipman_exists (clipman, dtext, DEFAULT, RAWTEXT)) {
          DBG("Text copy ..");
          clipman_add_clip (clipman, dtext, DEFAULT, RAWTEXT);
          clipman_array_remove_oldest (clipman);
        }
        g_free (dtext);
      } else {
        GdkPixbuf *image = gtk_clipboard_wait_for_image(defaultClip);
        if (image != NULL) {
          if (!clipman_exists (clipman, image, DEFAULT, IMAGE)) {
            DBG("Image copy ..");
            clipman_add_clip (clipman, image, DEFAULT, IMAGE);
            clipman_array_remove_oldest (clipman);
          }
          g_object_unref(image);
        }
      }
    }
  }

  /* PRIMARY - only if 'Add Selections' is ticked */
  if (clipboard == primaryClip && clipman->AddSelect) {
    // Program holding selection has closed - restore Primary clipboard
    if (reason == GDK_OWNER_CHANGE_CLOSE || reason == GDK_OWNER_CHANGE_DESTROY) {
      clipman_refill_clipboard (clipman, PRIMARY);
      return;
    }
     
    if (reason == GDK_OWNER_CHANGE_NEW_OWNER) {
      // Get selected - TEXT only
      ptext = gtk_clipboard_wait_for_text (primaryClip);
      // User has selected text.
      if (ptext != NULL) {
        if (!clipman_exists (clipman, ptext, PRIMARY, RAWTEXT)) {
          DBG("Text select done");
          clipman_add_clip (clipman, ptext, PRIMARY, RAWTEXT);
          clipman_array_remove_oldest (clipman);
        }
        g_free (ptext);
        
      // If an image has been selected, don't process it - but reset PrimaryIndex
      } else if (gtk_clipboard_wait_is_image_available(primaryClip)) {
        clipman->PrimaryIndex=-1;
      }
    }
  }

}

/* Called when -
  - user selects data
  - user copies data to clipboard
  - program holding a clipboard closes down
*/
static void clipboard_changed(GtkClipboard *clipboard, GdkEvent *event, ClipmanPlugin *clipman) {

  // Signal has been sent by this plugin
  if (IgnoreSignal) {
    DBG("Signal Ignored");
    IgnoreSignal=FALSE;
    return;
  }
  
  /* Note this extra effort is only required for the PRIMARY selects
     in some applications */
  if (clipboard == primaryClip && clipman->AddSelect) {
    GdkModifierType  state;
    gdk_window_get_pointer(NULL, NULL, NULL, &state);
    if (state & GDK_BUTTON1_MASK) {
      DBG("Left btn pressed");
      MouseSelecting=TRUE;
      return;  // not done yet
    } else if (state & GDK_SHIFT_MASK) {
      DBG("Shift key pressed");
      ShiftSelecting=TRUE;
      return;  // not done yet
    }
  }
       
  /* Reason the signal was sent */
  GdkOwnerChange reason = ((GdkEventOwnerChange*)event)->reason;
  clipman_clipboard_changed(clipboard, clipman, reason);

}

/* This runs every 0.5 seconds - minimize what it does. */
static gboolean clipman_timed_poll (ClipmanPlugin *clipman)
{
    // Nearly always, this is all this function will do ..
    if (!MouseSelecting && !ShiftSelecting) return TRUE;

    GdkModifierType  state;
    gdk_window_get_pointer(NULL, NULL, NULL, &state);
    if (MouseSelecting==TRUE && !(state & GDK_BUTTON1_MASK)) MouseSelecting=FALSE;
    if (ShiftSelecting==TRUE && !(state & GDK_SHIFT_MASK))   ShiftSelecting=FALSE;

    // Now that the selection is finished, run the necessary code ..
    if (!MouseSelecting && !ShiftSelecting) {
      DBG("Finished selecting");    
      clipman_clipboard_changed(primaryClip, clipman, GDK_OWNER_CHANGE_NEW_OWNER);
    }
    
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
                                                 (GSourceFunc) clipman_timed_poll,
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
    xfce_rc_write_bool_entry (rc, "AddSelect", clipman->AddSelect);
    
    xfce_rc_write_bool_entry (rc, "ItemNumbers",  clipman->ItemNumbers);

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

            /* Don't save the image */
            if (clip->datatype == IMAGE)
              continue;

            g_snprintf (name, 13, "clip_%d_text", i);
            xfce_rc_write_entry (rc, name, clip->data);

            g_snprintf (name, 13, "clip_%d_from", i);
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
    clipman->AddSelect     = xfce_rc_read_bool_entry (rc, "AddSelect", DEFADDSELECT);
    clipman->ItemNumbers      = xfce_rc_read_bool_entry (rc, "ItemNumbers",    DEFITEMNUMBERS);

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
                clipman_add_clip (clipman, value, PRIMARY, RAWTEXT);
            else
                clipman_add_clip (clipman, value, DEFAULT, RAWTEXT);

	    g_free (value);
        }
    }

    xfce_rc_close (rc);
}

static ClipmanPlugin *
clipman_new (XfcePanelPlugin *plugin)
{
    ClipmanPlugin *clipman;
    clipman = panel_slice_new0 (ClipmanPlugin);

    clipman->clips = g_ptr_array_new ();
    clipman->plugin = plugin;

    clipman->tooltip = gtk_tooltips_new ();
    g_object_ref (G_OBJECT (clipman->tooltip));

    clipman->DefaultIndex=-1;
    clipman->PrimaryIndex=-1;

    /* Load Settings, and possibly Data */
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
            G_CALLBACK(clipman_icon_clicked), clipman);

    /* Start the clipman_timed_poll function */
    /* TODO Run the timeout only if the plugin takes care of the selections */
    clipman->TimeoutId = g_timeout_add_full(G_PRIORITY_LOW,
                                            TIMER_INTERVAL,
                                            (GSourceFunc) clipman_timed_poll,
                                            clipman,
                                            (GDestroyNotify) clipman_reset_timeout);

    /* Connect to the clipboards */
    defaultClip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    primaryClip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);

    /* Respond to clipboard events - rather than polling twice a second */
  	g_signal_connect(G_OBJECT(defaultClip), "owner-change", G_CALLBACK(clipboard_changed), clipman);
	  g_signal_connect(G_OBJECT(primaryClip), "owner-change", G_CALLBACK(clipboard_changed), clipman);

    return clipman;
}

static void
clipman_free (XfcePanelPlugin *plugin,
              ClipmanPlugin   *clipman)
{
    gint        i;
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
    for (i=(gint)clipman->clips->len-1; i>=0; i--)
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

    panel_slice_free (ClipmanPlugin, clipman);
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
