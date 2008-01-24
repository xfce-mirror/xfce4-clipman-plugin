/*  $Id: clipman-dialogs.h 2395 2007-01-17 17:42:53Z nick $
 *
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
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

#ifndef CLIPMAN_DIALOGS_H
#define CLIPMAN_DIALOGS_H

G_BEGIN_DECLS

#define PLUGIN_WEBSITE "http://goodies.xfce.org/projects/panel-plugins/xfce4-clipman-plugin"

typedef struct _ClipmanOptions      ClipmanOptions;

struct _ClipmanOptions
{
  ClipmanPlugin        *clipman;

  GtkWidget            *ExitSave;
  GtkWidget            *IgnoreSelection;
  GtkWidget            *PreventEmpty;

  GtkWidget            *Behaviour;

  GtkWidget            *ItemNumbers;
  GtkWidget            *SeparateMenu;

  GtkWidget            *HistorySize;
  GtkWidget            *ItemChars;
};

void
clipman_configure_new (ClipmanPlugin *clipman_plugin);

G_END_DECLS

#endif
