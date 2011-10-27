/*
 *  common.h -- defines default values to use between the components
 *
 *  Copyright (c) 2009-2011 Mike Massonnet <mmassonnet@xfce.org>
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

#define DEFAULT_MAX_TEXTS_IN_HISTORY                    10
#define DEFAULT_MAX_IMAGES_IN_HISTORY                   1
#define DEFAULT_SAVE_ON_QUIT                            TRUE
#define DEFAULT_POPUP_AT_POINTER                        FALSE
#define DEFAULT_REVERSE_ORDER                           FALSE
#define DEFAULT_REORDER_ITEMS                           TRUE
#define DEFAULT_SKIP_ACTION_ON_KEY_DOWN                 FALSE
#define DEFAULT_ADD_PRIMARY_CLIPBOARD                   FALSE
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

#define XFCE_CLIPMAN_SELECTION    "XFCE_CLIPMAN_SELECTION"
#define XFCE_CLIPMAN_MESSAGE      "MENU"

/*
 * Action Groups
 */

#define ACTION_GROUP_SELECTION  0
#define ACTION_GROUP_MANUAL     1
