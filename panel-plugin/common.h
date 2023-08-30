/*
 *  common.h -- defines default values to use between the components
 *
 *  Copyright (c) 2009-2012 Mike Massonnet <mmassonnet@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Default values
 */

#ifndef __CLIPMAN_COMMON_H__
#define __CLIPMAN_COMMON_H__

#include <gdk/gdk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#else
#define GDK_IS_X11_DISPLAY(display) FALSE
#endif

#define DEFAULT_MAX_MENU_ITEMS                          15
#define DEFAULT_MAX_TEXTS_IN_HISTORY                    100
#define DEFAULT_MAX_IMAGES_IN_HISTORY                   1
#define DEFAULT_SAVE_ON_QUIT                            TRUE
#define DEFAULT_POPUP_AT_POINTER                        FALSE
#define DEFAULT_REVERSE_ORDER                           FALSE
#define DEFAULT_REORDER_ITEMS                           TRUE
#define DEFAULT_SKIP_ACTION_ON_KEY_DOWN                 FALSE
#define DEFAULT_ADD_PRIMARY_CLIPBOARD                   FALSE
#define DEFAULT_PERSISTENT_PRIMARY_CLIPBOARD            FALSE
#define DEFAULT_SHOW_QR_CODE                            FALSE
#define DEFAULT_HISTORY_IGNORE_PRIMARY_CLIPBOARD        TRUE
#define DEFAULT_ENABLE_ACTIONS                          FALSE

/*
 * Modes for paste-on-activate
 */
#define PASTE_INACTIVE  0
#define PASTE_CTRL_V    1
#define PASTE_SHIFT_INS 2

/*
 * Selection for the popup command
 */

#define XFCE_CLIPMAN_SELECTION        "XFCE_CLIPMAN_SELECTION"
#define XFCE_CLIPMAN_MESSAGE          "MENU"
#define XFCE_CLIPMAN_ACTION_MESSAGE   "ACTIONS"

/*
 * Action Groups
 */

#define ACTION_GROUP_SELECTION  0
#define ACTION_GROUP_MANUAL     1

void                    clipman_common_show_info_dialog    (void);
void                    clipman_common_show_warning_dialog (void);
gchar *                 clipman_common_shorten_preview     (const gchar *text);
void                    clipman_common_paste_on_activate   (guint        paste_on_activate);

#endif /* !__CLIPMAN_COMMON_H__ */
