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
	}

	return 1;
}

TEST(DNS, Init)
{
	ape_global * g_ape;

	g_ape = NULL;
	g_ape = native_netlib_init();
	EXPECT_EQ(g_ape->dns.sockets.size, 32);
	EXPECT_EQ(g_ape->dns.sockets.used, 0);

	//native_netlib_destroy(g_ape);
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

	g_ape = native_netlib_init();

	ape_running = g_ape->is_running = 1;
	dns_state = ape_gethostbyname("nidium.com", shutdown_loop_on_resolve, (void*)"212.83.162.183", g_ape);
	events_loop(g_ape);

	ape_running = g_ape->is_running = 1;
	dns_state = ape_gethostbyname("212.83.162.183", shutdown_loop_on_resolve, (void*)"212.83.162.183", g_ape);
	events_loop(g_ape);

	//native_netlib_destroy(g_ape);
}
