#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <native_netlib.h>

static int counter;

static void shutdown_loop_on_timer( void * param, int *lastcall)
{
	int *val;

	val = (int*)param;
	printf("%d %d\n", *val, *lastcall);
	if (*lastcall == 1||
		*val == 200) {
		counter--;
	}
	if (*lastcall == 1||
		*val == 100) {
		counter--;
	}
	if (counter == 0) {
		ape_running = 0;
	}
}

TEST(Timers, Simple)
{
	ape_global * g_ape;
	struct _ticks_callback *timer1, *timer2, * dummy;
	int id, val1, val2;

	val1 = 20;
	val2 = 10;
	g_ape = native_netlib_init();

	id = get_first_timer_ms(g_ape);
	EXPECT_EQ(id, -1);

	timer1 = add_timeout(val1, NULL, NULL, g_ape);
	EXPECT_EQ(timer1->identifier, 0);
	id = get_first_timer_ms(g_ape);
	EXPECT_EQ(id, val1);
 
	timer2 = add_timeout(val2, NULL, NULL, g_ape);
	EXPECT_EQ(timer2->identifier, 1);
	id = get_first_timer_ms(g_ape);
	EXPECT_EQ(id, val2);
	
	dummy = get_timer_identifier(1, g_ape);
	EXPECT_EQ(dummy, timer2);

	del_timer_identifier(1, g_ape);
	id = get_first_timer_ms(g_ape);
	EXPECT_EQ(id, val2);
	
	timers_free(g_ape);

	id = get_first_timer_ms(g_ape);	
	EXPECT_EQ(id, -1);

//	native_netlib_destroy(g_ape);
}

TEST(Timers, Timeout)
{
	ape_global * g_ape;
	struct _ticks_callback *timer1, *timer2;
	int id;
	counter = 2;
	int val1, val2;

	val1 = 1200;
	val2 = 1100;

	g_ape = native_netlib_init();

	timer1 = add_timeout(val1, (void*)shutdown_loop_on_timer, &val1, g_ape);
	timer2 = add_timeout(val2, (void*)shutdown_loop_on_timer, &val2, g_ape);
	
	ape_running = g_ape->is_running = 0;
	//@FIXME: process tick is not called in the loop is not called
	//events_loop(g_ape);

	id = get_first_timer_ms(g_ape);	
	EXPECT_EQ(id, -1);

//	native_netlib_destroy(g_ape);
}

