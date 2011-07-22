/*
 * Mex - a media explorer
 *
 * Copyright © 2011 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifndef __DBUS_CLIENT_H__
#define __DBUS_CLIENT_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _HTTPDBusInterface HTTPDBusInterface;

struct _HTTPDBusInterface 
{
  GDBusConnection *connection;
  GDBusProxy *dbusinput_proxy;
  GDBusProxy *mediaplayer_proxy;
};

HTTPDBusInterface *httpdbus_interface_new (void);
void httpdbus_interface_free (HTTPDBusInterface *dbus_interface);

gchar *httpdbus_media_player_get (HTTPDBusInterface *dbus_interface,
                                  gchar *get);

void
httpdbus_media_player_set_uri (HTTPDBusInterface *dbus_interface,
                               gchar *uri);

void
httpdbus_send_keyvalue (HTTPDBusInterface *dbus_interface, gint keyval);


G_END_DECLS

#endif