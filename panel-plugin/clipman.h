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

#ifndef CLIPMAN_H
#define CLIPMAN_H

G_BEGIN_DECLS

/* Dialog settings */
#define BORDER          8

/* History settings: default, min and max */
#define DEFHISTORY      10
#define MAXHISTORY      100
#define MINHISTORY      5

/* Character settings: default, min and max */
#define DEFCHARS        30
#define MAXCHARS        200
#define MINCHARS        10

/* Default options */
#define DEFEXITSAVE     FALSE
#define DEFIGNORESELECT TRUE
#define DEFPREVENTEMPTY TRUE
#define DEFBEHAVIOUR    1

#define DEFITEMNUMBERS  FALSE
#define DEFSEPMENU      FALSE

/* milisecond to check the clipman(s) */
#define TIMER_INTERVAL  500

typedef enum
{
    PRIMARY = 0,
    DEFAULT,
}
ClipboardType;

typedef enum
{
    NORMAL = 0,
    STRICTLY,
}
ClipboardBehaviour;

typedef struct
{
    XfcePanelPlugin *plugin;

    GtkWidget    *icon;
    GtkWidget    *button;
    GtkTooltips  *tooltip;
    
    GPtrArray    *clips;
    
    gint          TimeoutId;
    gboolean      killTimeout;
    
    gboolean      ExitSave;
    gboolean      IgnoreSelect;
    gboolean      PreventEmpty;
    
    ClipboardBehaviour Behaviour;
    
    gboolean      ItemNumbers;
    gboolean      SeparateMenu;
    
    guint         HistoryItems;
    guint         MenuCharacters;
}
ClipmanPlugin;

typedef struct
{
    gchar        *text;
    gchar        *title;        /* I've added the title to save
                                 * some time when opening the menu */
    ClipboardType fromtype;
}
ClipmanClip;

typedef struct
{
    ClipmanPlugin  *clipman;
    ClipmanClip    *clip;
}
ClipmanAction;

void
clipman_check_array_len        (ClipmanPlugin *clipman);

void
clipman_regenerate_titles      (ClipmanPlugin *clipman);

void
clipman_save                   (XfcePanelPlugin *plugin, ClipmanPlugin *clipman);

void
clipman_remove_selection_clips (ClipmanPlugin *clipman);

GtkClipboard *primaryClip, *defaultClip;

G_END_DECLS

#endif /* CLIPMAN_H */
