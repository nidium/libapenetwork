/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <native_netlib.h>
#include <ape_events_loop.h>

static int counter;

static int shutdown_loop_on_timer(void *param)
{
	int *val;

	val = (int*)param;
	if (*val == 200) {
		counter--;
	}
	if (*val == 100) {
		counter--;
	}
	if (counter == 0) {
		ape_running = 0;
	}
	return 0;
}

static int shutdown_loop_on_timeout(void *param)
{
	int *val;

	val = (int*)param;
	if (*val == 199) {
		counter--;
	}
	if (counter == 0) {
		APE_loop_stop();
	}
	return 10;
}


TEST(TimersNg, Timer)
{
	ape_global * g_ape;
	ape_timer_t *timer1, *timer2, *dummy;
	int val1, val2, *dv;

	val1 = 20;
	val2 = 10;

	g_ape = APE_init();

	// see if we can find some none exisiting id
	dummy = APE_timer_getbyid(g_ape, 1);
	EXPECT_TRUE(dummy == NULL);

	// see if we can find some none exisiting id, 0 is the first right?
	dummy = APE_timer_getbyid(g_ape, 0);
	EXPECT_TRUE(dummy == NULL);

	// let's add a timer
	timer1 = APE_timer_create(g_ape, val1, shutdown_loop_on_timer, (void*)&val1);
	EXPECT_EQ(APE_timer_getid(timer1), 1);
	dummy = APE_timer_getbyid(g_ape, 1);
	dv = (int*)APE_timer_getarg(dummy);
	EXPECT_EQ(*dv, val1);
	
	// let's add a another timer 
	timer2 = APE_timer_create(g_ape, val2, shutdown_loop_on_timer, (void*)&val2);
	EXPECT_EQ(APE_timer_getid(timer2), 2);
	dummy = APE_timer_getbyid(g_ape, 2);
	dv = (int*)APE_timer_getarg(dummy);
	EXPECT_EQ(*dv, val2);

	// can we still find the correct one?
	dummy = APE_timer_getbyid(g_ape, 2);
	EXPECT_EQ(dummy, timer2);

	// let's delete a timer
	APE_timer_destroy(g_ape, dummy);
	dummy = APE_timer_getbyid(g_ape, 2);
	EXPECT_TRUE(dummy == NULL);

	//let's add it again
	timer2 = APE_timer_create(g_ape, val2, shutdown_loop_on_timer, (void*)&val2);
	EXPECT_EQ(APE_timer_getid(timer2), 3);

	ape_running = g_ape->is_running = 0;
	APE_loop_run(g_ape);

	//all timers should be clear
	dummy = APE_timer_getbyid(g_ape, 2);
	EXPECT_TRUE(dummy == NULL);

	//Add one and clear it
	timer2 = APE_timer_create(g_ape, val2, shutdown_loop_on_timer, (void*)&val2);
	EXPECT_EQ(APE_timer_getid(timer2), 4);
	timer1 = APE_timer_create(g_ape, val2, shutdown_loop_on_timer, (void*)&val2);
	EXPECT_EQ(APE_timer_getid(timer1), 5);
	APE_timer_clearbyid(g_ape, APE_timer_getid(timer2), 4);

	dummy = APE_timer_getbyid(g_ape, 5);
	EXPECT_TRUE(dummy == timer1);

	APE_destroy(g_ape);

}

TEST(TimersNg, Interval)
{
	ape_global * g_ape;
	ape_timer_t *interval, *dummy;
	int val;

	val = 199;
	g_ape = APE_init();

	// let's add a timeout
	interval = APE_timer_create(g_ape, 1, shutdown_loop_on_timeout, &val);
	EXPECT_EQ(APE_timer_getid(interval), 1);
	
	counter = 200;
	ape_running = 1;
	APE_loop_run(g_ape);

	//all timers should be clear
	dummy = APE_timer_getbyid(g_ape, 2);
	EXPECT_TRUE(dummy == NULL);
	
	APE_destroy(g_ape);

}

