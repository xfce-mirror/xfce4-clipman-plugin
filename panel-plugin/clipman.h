/*  $Id$
 *
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *  Copyright (c) 2008-2009 David Collins <david.8.collins@gmail.com>
 *  Copyright (c) 2009      Mike Massonnet <mmassonnet@xfce.org>
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
#define DEFADDSELECT    FALSE
#define DEFITEMNUMBERS  FALSE

/* Milisecond to check the clipboards(s) */
#define TIMER_INTERVAL  500

#define CLIPIMAGETITLE   "**IMAGE**"

typedef enum
{
    PRIMARY = 0,
    DEFAULT
}
ClipboardType;

typedef enum
{
    RAWTEXT,
    IMAGE
}
ClipDataType;

typedef enum
{
    PLAIN,
    BOLD,
    ITALICS
}
ClipMenuFormat;

typedef struct
{
    XfcePanelPlugin *plugin;

    GtkWidget    *icon;
    GtkWidget    *button;
    GtkTooltips  *tooltip;

    gint          TimeoutId;
    gboolean      killTimeout;

    gboolean      ExitSave;
    gboolean      AddSelect;

    GPtrArray    *clips;
    gint          DefaultIndex;
    gint          PrimaryIndex;
    GSList       *actions;

    gboolean      ItemNumbers;

    guint         HistoryItems;
    guint         MenuCharacters;
}
ClipmanPlugin;

typedef struct
{
    void         *data;
    gchar        *title;
    GdkPixbuf    *preview;
    ClipDataType  datatype;
}
ClipmanClip;

typedef struct
{
    ClipmanPlugin  *clipman;
    ClipmanClip    *clip;
    gint            index;
}
ClipmanAction;

void
clipman_array_remove_oldest   (ClipmanPlugin *clipman);

void
clipman_save                  (XfcePanelPlugin *plugin, ClipmanPlugin *clipman);


G_END_DECLS

#endif /* CLIPMAN_H */

