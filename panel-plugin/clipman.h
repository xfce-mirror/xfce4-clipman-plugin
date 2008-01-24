/*  $Id: clipman.h 2395 2007-01-17 17:42:53Z nick $
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

#ifndef CLIPMAN_H
#define CLIPMAN_H

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfcegui4/libxfcegui4.h>

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

/* Milisecond to check the clipboards(s) */
#define TIMER_INTERVAL  1000

typedef enum
{
  DEFAULT,
  PRIMARY
} ClipboardType;

typedef enum
{
  NORMAL,
  STRICTLY
} ClipboardBehavior;

typedef struct _ClipmanPlugin       ClipmanPlugin;
typedef struct _ClipmanClips        ClipmanClips;
typedef struct _ClipmanClip         ClipmanClip;

struct _ClipmanPlugin
{
  XfcePanelPlugin      *panel_plugin;
  ClipmanClips         *clipman_clips;

  GtkWidget            *button;
  GtkWidget            *icon;
  GtkWidget            *menu;

  gboolean              menu_separate_clipboards;
  gboolean              menu_item_show_number;
  gint                  menu_item_max_chars;
};

struct _ClipmanClips
{
  ClipmanPlugin        *clipman_plugin;

  GtkClipboard         *default_clipboard;
  GtkClipboard         *primary_clipboard;

  gint                  timeout;

  GSList               *history;

  ClipboardBehavior     behavior;
  gint                  history_length;
  gboolean              save_on_exit;
  gboolean              ignore_primary;
  gboolean              prevent_empty;
};

struct _ClipmanClip
{
    ClipboardType       type;
    gchar              *text;
    gchar              *short_text; /* Saves cycles to add menu items */
};

G_END_DECLS

#endif /* CLIPMAN_H */
