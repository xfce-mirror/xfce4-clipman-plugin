/*
 * Copyright (C) 2023 Gaël Bonithon <gael@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XCP_CLIPBOARD_MANAGER_H__
#define __XCP_CLIPBOARD_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define XCP_TYPE_CLIPBOARD_MANAGER (xcp_clipboard_manager_get_type ())
G_DECLARE_INTERFACE (XcpClipboardManager, xcp_clipboard_manager, XCP, CLIPBOARD_MANAGER, GObject)

struct _XcpClipboardManagerInterface
{
  GTypeInterface g_iface;
};

XcpClipboardManager *
xcp_clipboard_manager_get (void);

G_END_DECLS

#endif /* !__XCP_CLIPBOARD_MANAGER_H__ */
