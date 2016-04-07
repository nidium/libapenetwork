/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <native_netlib.h>

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
		ape_running = 0;
	}
	return 10;
}


TEST(TimersNg, Timer)
{
	ape_global * g_ape;
	ape_timer *timer1, *timer2, *dummy;
	int val1, val2, *dv;
	int flags;

	val1 = 20;
	val2 = 10;
	flags = APE_TIMER_IS_CLEARED;
	g_ape = native_netlib_init();

	// see if we can find some none exisiting id
	dummy = get_timer_by_id(&g_ape->timersng, 1);
	EXPECT_TRUE(dummy == NULL);

	// see if we can find some none exisiting id, 0 is the first right?
	dummy = get_timer_by_id(&g_ape->timersng, 0);
	EXPECT_TRUE(dummy == NULL);

	// let's add a timer
	timer1 = add_timer(&g_ape->timersng, val1, shutdown_loop_on_timer, (void*)&val1);
	EXPECT_EQ(timer1->identifier, 1);
	dummy = get_timer_by_id(&g_ape->timersng, 1);
	dv = (int*)dummy->arg;
	EXPECT_EQ(*dv, val1);
	
	// let's add a another timer 
	timer2 = add_timer(&g_ape->timersng, val2, shutdown_loop_on_timer, (void*)&val2);
	EXPECT_EQ(timer2->identifier, 2);
	dummy = get_timer_by_id(&g_ape->timersng, 2);
	dv = (int*)dummy->arg;
	EXPECT_EQ(*dv, val2);

	// can we still find the correct one?
	dummy = get_timer_by_id(&g_ape->timersng, 2);
	EXPECT_EQ(dummy, timer2);

	// let's delete a timer
	dummy = del_timer(&g_ape->timersng, dummy);
	EXPECT_EQ(dummy, timer1);
	dummy = get_timer_by_id(&g_ape->timersng, 2);
	EXPECT_TRUE(dummy == NULL);

	//let's add it again
	timer2 = add_timer(&g_ape->timersng, val2, shutdown_loop_on_timer, (void*)&val2);
	EXPECT_EQ(timer2->identifier, 3);

	ape_running = g_ape->is_running = 0;
	events_loop(g_ape);

	//all timers should be clear
	dummy = get_timer_by_id(&g_ape->timersng, 2);
	EXPECT_TRUE(dummy == NULL);

	//Add one and clear it
	timer2 = add_timer(&g_ape->timersng, val2, shutdown_loop_on_timer, (void*)&val2);
	EXPECT_EQ(timer2->identifier, 4);
	timer1 = add_timer(&g_ape->timersng, val2, shutdown_loop_on_timer, (void*)&val2);
	EXPECT_EQ(timer1->identifier, 5);
	clear_timer_by_id(&g_ape->timersng, timer2->identifier, 4);
	dummy = get_timer_by_id(&g_ape->timersng, 4);
	EXPECT_EQ(dummy->flags & APE_TIMER_IS_CLEARED, flags);
	dummy = get_timer_by_id(&g_ape->timersng, 5);
	EXPECT_TRUE(dummy == timer1);

	native_netlib_destroy(g_ape);

}

TEST(TimersNg, Interval)
{
	ape_global * g_ape;
	ape_timer *interval, *dummy;
	int val;

	val = 199;
	g_ape = native_netlib_init();

	// let's add a timeout
	interval = add_timer(&g_ape->timersng, 1, shutdown_loop_on_timeout, &val);
	EXPECT_EQ(interval->identifier, 1);
	
	counter = 200;
	ape_running = 1;
	events_loop(g_ape);

	//all timers should be clear
	dummy = get_timer_by_id(&g_ape->timersng, 2);
	EXPECT_TRUE(dummy == NULL);
	
	native_netlib_destroy(g_ape);

}

