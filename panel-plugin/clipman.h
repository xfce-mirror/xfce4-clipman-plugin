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
 
/* Dialog settings */
#define BORDER			8

/* History settings: default, min and max */
#define DEFHISTORY		10
#define MAXHISTORY		100
#define MINHISTORY		5

/* Character settings: default, min and max */
#define DEFCHARS		30
#define MAXCHARS		200
#define MINCHARS		10

/* Default options */
#define DEFEXITSAVE		FALSE
#define DEFIGNORESELECT		FALSE
#define DEFPREVENTEMPTY		TRUE
#define DEFFROMONETYPE		TRUE

#define DEFITEMNUMBERS		FALSE
#define DEFSEPARATEBOARDS	FALSE
#define DEFCOLOREDITEMS		TRUE

#define DEFPRICOLOR		"#000099"
#define DEFDEFCOLOR		"#990000"
#define DEFNUMCOLOR		"#999999"

/* milisecond to check the clipman(s) */
#define TIMER_INTERVAL		500

typedef enum
{
	FROM_PRIMIRY,
	FROM_DEFAULT,
}
ClipboardType;

typedef struct
{
	XfcePanelPlugin *plugin;

	GtkWidget	*menu;
	GtkWidget	*button;
	GtkTooltips	*tooltip;
	
	GPtrArray	*clips;
	
	gint		timeId;
	gboolean	killTimeout;
	
	guint		ExitSave:1;
	guint		IgnoreSelect:1;
	guint		PreventEmpty:1;
	guint		FromOneType:1;
	
	guint		ItemNumbers:1;
	guint		SeparateBoards:1;
	guint		ColoredItems:1;
	
	gint		HistoryItems;
	gint		MenuCharacters;
	
	gchar		*DefColor;
	gchar		*PriColor;
	gchar		*NumColor;
}
ClipmanPlugin;

typedef struct
{
	ClipmanPlugin	*clipman;
	
	GtkWidget	*G_ExitSave;
	GtkWidget	*G_IgnoreSelection;
	GtkWidget	*G_PreventEmpty;
	GtkWidget	*G_FromOneType;
	GtkWidget	*G_FromOneType_dup;
	
	GtkWidget	*M_ItemNumbers;
	GtkWidget	*M_SepBoards;
	GtkWidget	*M_ColoredItems;
	
	GtkWidget	*N_HistorySize;
	GtkWidget	*N_ItemChars;
}
ClipmanOptions;

typedef struct
{
	gchar		*text;
	gchar		*title;		/* I've added the title to save
					 * some time when opening the menu */
	ClipboardType	fromtype;
}
ClipmanClip;

typedef struct
{
	ClipmanPlugin	*clipman;
	ClipmanClip	*clip;
}
ClipmanAction;

void
clipman_check_array_len (ClipmanPlugin *clipman);

void
clipman_regenerate_titles (ClipmanPlugin *clipman);

void
clipman_save (XfcePanelPlugin *plugin, ClipmanPlugin *clipman);

GtkClipboard *primaryClip, *defaultClip;
