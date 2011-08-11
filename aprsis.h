/*
 * aprsis.c
 * Copyright (C) 2011 Gordon JC Pearce <gordon@gjcp.net>
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
 
#include <gio/gio.h>

enum { APRSIS_DISCONNECTED, APRSIS_CONNECTING, APRSIS_CONNECTED, APRSIS_LOGIN };

typedef struct _aprsis_ctx {
    GSocket *skt;
    guint state;
	int sockfd;
	char *host;
	char *port;
	char *user;
	char *pass;
} aprsis_ctx;

int aprsis_write(aprsis_ctx *ctx, char *buf, size_t len);
