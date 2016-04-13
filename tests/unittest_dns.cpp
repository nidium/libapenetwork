/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ares.h>

#include <native_netlib.h>
#include <ape_dns.h>
#include <ape_events_loop.h>

static int shutdown_loop_on_resolve( const char *ip, void * arg, int status)
{
	if (status == ARES_SUCCESS && strcmp(ip, (char*) arg ) == 0) {
		ape_running = 0;
	} else {
		exit(2);
	}

	return 1;
}

TEST(DNS, Init)
{
	ape_global * g_ape;

	g_ape = APE_init();
	EXPECT_EQ(g_ape->dns.sockets.size, 32);
	EXPECT_EQ(g_ape->dns.sockets.used, 0);

	APE_destroy(g_ape);
}

TEST(DNS, Invalidate)
{
	ape_dns_state state;

	state.invalidate = 2;
	ape_dns_invalidate(&state);
	EXPECT_EQ(state.invalidate, 1);
}

TEST(DNS, Resolve)
{
	ape_global * g_ape;
	ape_dns_state *dns_state;

	g_ape = APE_init();

	ape_running = g_ape->is_running = 1;
	dns_state = ape_gethostbyname("nidium.com", shutdown_loop_on_resolve, (void*)"212.83.162.183", g_ape);
	APE_loop_run(g_ape);

	ape_running = g_ape->is_running = 1;
	dns_state = ape_gethostbyname("212.83.162.183", shutdown_loop_on_resolve, (void*)"212.83.162.183", g_ape);
	APE_loop_run(g_ape);

	APE_destroy(g_ape);
}

