/*
 * aprsbeacon.c
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

#include <glib.h>
#include "aprsis.h"

GMainLoop *main_loop = NULL;

int main (int argc, gchar *argv[])
{
	g_print ("[INFO] Starting example...\n");
//	app_init (argv[1]);
	g_print ("[INFO] Example started.\n");

    aprsis_ctx ctx;
    ctx.user = "mm0yeq";
    ctx.pass = "-1";
    ctx.state = APRSIS_DISCONNECTED;
    
    g_thread_init(NULL);
    g_type_init();
    
    start_aprsis(&ctx);

	main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (main_loop);

	return 0;
}


/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/* vim:set et sw=4 ts=4 cino=t0,(0: */

