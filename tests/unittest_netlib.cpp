#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

#include <ares.h>

#include <ape_dns.h>
#include <ape_events_loop.h>
#include <native_netlib.h>

int ape_running;

static int shutdown_loop_on_resolve( const char *ip, void * arg, int status)
{
	if ( status == ARES_SUCCESS &&
		strcmp(ip, "212.83.162.183") == 0 &&
		arg == NULL) {
		ape_running = 0;
	}

	return 1;
}


TEST(Netlib, Simple)
{
	ape_global * g_ape;
	ape_dns_state *dns_state;

	g_ape = NULL;
	ape_running = 1;

	g_ape = native_netlib_init();
	EXPECT_TRUE(g_ape != NULL);

	dns_state = ape_gethostbyname( "nidium.com", shutdown_loop_on_resolve, NULL, g_ape);
	events_loop(g_ape);

	native_netlib_destroy(g_ape);
}

