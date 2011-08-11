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

#include <stdlib.h>
#include <glib.h>
#include <gps.h>
#include <errno.h>
#include <math.h>
#include "gpsdclient.h"
#include "aprsis.h"

GMainLoop *main_loop = NULL;

static struct gps_data_t gpsdata;
static struct fixsource_t source;
static guint sid3; // FIXME
static guint flags = WATCH_ENABLE;
static GIOChannel *gpsd_io_channel =NULL;

double speed, track, secs_since_beacon, hcsb, beacon_time;
double turn_threshold, beacon_rate, old_track=-1;
static gdouble next_time;

float high_speed = 60;
float fast_rate = 180;
float slow_speed = 5;
float slow_rate = 1800;
float min_turn_time = 15;
float min_turn_angle = 30;
float turn_slope = 240;

static gboolean gpsd_data_cb(GIOChannel *src, GIOCondition condition, gpointer data) {
	int ret;
	gdouble time, speed, lat, lon;

	ret = gps_read(&gpsdata);

	if (!isnan(gpsdata.fix.latitude)) {
        lat = gpsdata.fix.latitude;
        lon = gpsdata.fix.longitude;
	}
	if (!isnan(gpsdata.fix.speed)) speed=gpsdata.fix.speed*2.24;
	if (!isnan(gpsdata.fix.track)) track=gpsdata.fix.track;
	
	
	if ((beacon_time==0) || (secs_since_beacon<0)) {
		printf("init beacon time\n");
		beacon_time=time;
	}
	
	time = gpsdata.fix.time;

	if (time>next_time) {
		printf("3 min point\n");
		next_time = time+180;
	}
#if 0
printf("\n===============================================\n");	
printf("ssb: %f\ntime until beacon: %f\nbeacon_rate: %f\n", secs_since_beacon, beacon_rate-secs_since_beacon, beacon_rate);
printf("track: %f\noldtrack: %f\n", track, old_track);
printf("hcsb: %f\ntt: %f\nmtt: %f\n", hcsb, turn_threshold, min_turn_time);
#endif 

	// calculate smart beacon
	secs_since_beacon = time-beacon_time;
	if (speed < slow_speed) {
		//printf("low speed = %f\n", speed);
		beacon_rate = slow_rate;
	} else {
		if (speed > high_speed) {
			//printf("high speed = %f\n", speed);
			beacon_rate = fast_rate;
		} else {
			beacon_rate = fast_rate * high_speed/speed;
		}
		if (old_track == -1) old_track = track;
		hcsb = fabs(track - old_track);
		if (hcsb > 180) hcsb = 360 - hcsb;
		turn_threshold = min_turn_angle + turn_slope / speed;
		if (hcsb > turn_threshold && secs_since_beacon > min_turn_time) {
			secs_since_beacon = beacon_rate;
		}
	
	}
	if (secs_since_beacon >= beacon_rate) {
		//printf("***************************************** BEACON\n");
        printf("smart beacon\n");
		beacon_time = time;
		old_track=track;
	}
	
	
	return TRUE;
	
}

int main (int argc, gchar *argv[])
{

    GError *error = NULL;
	GIOChannel *gio_read;

    aprsis_ctx ctx;
    ctx.user = "mm0yeq";
    ctx.pass = "-1";
    ctx.state = APRSIS_DISCONNECTED;
    
    g_thread_init(NULL);
    g_type_init();
    
    g_message("opening aprs-is");
    //start_aprsis(&ctx);

    g_message("opening gpsd");
	// Connect to GPS (FIXME move to thread)
    if (gps_open("localhost", "2947", &gpsdata) != 0) {
		printf("no gpsd: %d, %s\n", errno, gps_errstr(errno));
		exit(2);
    }
	gps_stream(&gpsdata, flags, source.device);
	printf("%x\n", source.device);
	gpsd_io_channel = g_io_channel_unix_new(gpsdata.gps_fd);
	g_io_channel_set_flags(gpsd_io_channel, G_IO_FLAG_NONBLOCK, NULL);

	sid3 = g_io_add_watch(gpsd_io_channel, G_IO_IN | G_IO_PRI, gpsd_data_cb, NULL);

	main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (main_loop);

	return 0;
}


/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/* vim:set et sw=4 ts=4 cino=t0,(0: */

