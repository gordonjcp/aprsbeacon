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

#include "aprsis.h"
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <stdio.h>

static guint reconnect_timer;
static GIOChannel *aprsis_io;

static GError *aprsis_connect(aprsis_ctx *ctx) {
	// connect to an APRS-IS server
	// return GError

	GError *err = NULL;
	GSocketClient *client = NULL;
	client = g_socket_client_new();
	// FIXME don't hardcode the port and server

	GSocketConnection *conn = g_socket_client_connect_to_host(client, "www.gjcp.net", 14580, NULL, &err); // FIXME needs to convert string to int
	
	if (conn) {
		ctx->skt = g_socket_connection_get_socket(conn);
	}
	if (ctx->skt) {
		ctx->sockfd = g_socket_get_fd(ctx->skt);
	}

	if(err) {
		g_message ("%s", err->message);
		ctx->state = APRSIS_DISCONNECTED;
	}
	return err;
}

int aprsis_read(aprsis_ctx *ctx, char *buf, size_t len) {
	// read a line from the GSocket, and maybe log it to a file
	GError *err;
	int count = g_socket_receive(ctx->skt, buf, len, NULL, &err);
	return count;
}

int aprsis_write(aprsis_ctx *ctx, char *buf, size_t len) {
	// send a line to the GSocket
	GError *err;
	int count = g_socket_send(ctx->skt, buf, len, NULL, &err);
	return count;
}

#define APRSIS_LOGIN "user %s pass %s vers aprsmap 0.0 filter r/55/-4/600"
static gboolean aprsis_got_packet(GIOChannel *gio, GIOCondition condition, gpointer data) {
	// callback when GIOChannel tells us there's an APRS packet to be handled
	GIOStatus ret;
	GError *err = NULL;
	gchar *msg;
	gsize len;
	aprsis_ctx *ctx = (aprsis_ctx *) data;

	if (condition & G_IO_HUP)
		g_message ("Read end of pipe died!");   // FIXME - handle this more gracefully

	if (condition & G_IO_ERR) {
		g_message ("IO error");
		return FALSE;
	}
		
	ret = g_io_channel_read_line (gio, &msg, &len, NULL, &err);
	if (ret == G_IO_STATUS_ERROR) {
		g_message("Error reading: %s", err->message);
		ctx->state = APRSIS_DISCONNECTED;
		return FALSE;
	}
	if (ret == G_IO_STATUS_EOF) {
		g_message("EOF (server disconnected)");
		ctx->state = APRSIS_DISCONNECTED;
		return FALSE; // shut down the callback, for now 
	}
	
	if (msg[0] == '#') {
		g_printf("can ignore comment message: %s\n", msg);
	} else if (ctx->state == APRSIS_CONNECTED) {
		sprintf(msg, APRSIS_LOGIN"\n", "mm0yeq", "19367");
		printf("sending %s\n",msg);
		ret = g_io_channel_write_chars (gio, msg, -1, NULL, &err);
        if (ret == G_IO_STATUS_ERROR) {
        
                g_error ("Error writing: %s\n", err->message);
		} else {
			ctx->state = APRSIS_LOGIN;
		}
	} else {
		printf("msg=%s\n",msg);
	
		//process_packet(msg);
	}

	g_free(msg);
	return TRUE;
}


static gboolean
aprsis_io_error(GIOChannel *src, GIOCondition condition, void *ptr)
{
	g_printf("*** %s(): \n",__PRETTY_FUNCTION__);
	aprsis_ctx *ctx = ptr;
	ctx->state = APRSIS_DISCONNECTED;
	//reconnect_timer = g_timeout_add_seconds(5, aprsis_reconnect, ctx);
	return FALSE;
}


int aprsis_login(aprsis_ctx *ctx) {
	// wait for prompt, send filter message
	char buf[256];
	int n;

	// note that this doesn't *actually* check what the prompt is
	bzero(&buf, 256);
	n = aprsis_read(ctx, buf, 256);
	if (n<0) {
		g_error("couldn't read from socket");
	}
	g_printf("got: %s",buf);

	sprintf(buf, APRSIS_LOGIN"\n", "mm0yeq", "19367");
	g_printf("sending: %s", buf);
	aprsis_write(ctx, buf, strlen(buf));
	bzero(&buf, 256);
	n = aprsis_read(ctx, buf, 256);
	if (n<0) {
		g_error("couldn't read from socket");
	}
	g_message("got: %s",buf);
	
	return 0;
}


static void start_aprsis_thread(void *ptr) {
// APRS connection thread
// waits for APRS-IS server to connect, then logs in

    GError *error = NULL;
	aprsis_ctx *ctx = ptr;

	// FIXME don't hardcode the timer
	//if (!reconnect_timer) reconnect_timer = g_timeout_add_seconds(10, aprsis_reconnect, ctx);
	
	if (aprsis_connect(ctx)) {
		g_message("failed to connect");
		return;
	}

	g_message("logging in...");
	aprsis_login(ctx);
	
	if (reconnect_timer) {
		g_source_remove (reconnect_timer);
		reconnect_timer = 0;
	}

	aprsis_io = g_io_channel_unix_new (ctx->sockfd);
    g_io_channel_set_encoding(aprsis_io, NULL, &error);
    if (!g_io_add_watch (aprsis_io, G_IO_IN, aprsis_got_packet, ctx))
        g_error ("Cannot add watch on GIOChannel G_IO_IN");
    
   
    if (!g_io_add_watch (aprsis_io,  G_IO_ERR | G_IO_HUP, aprsis_io_error, ctx))
        g_error ("Cannot add watch on GIOChannel G_IO_ERR or G_IO_HUP");

	ctx->state = APRSIS_CONNECTED;
	
}


void start_aprsis(aprsis_ctx *ctx) {
	// prepare the APRS-IS connection thread

	if (ctx->state != APRSIS_DISCONNECTED) return;

	// remove the IO channel and watch
	if (reconnect_timer) {
		g_source_remove (reconnect_timer);
		reconnect_timer = 0;
	}
	if (aprsis_io) {
		g_io_channel_unref (aprsis_io);
		aprsis_io = NULL;
	}
	g_thread_create((GThreadFunc) start_aprsis_thread, ctx, FALSE, NULL);
}



