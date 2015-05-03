#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <native_netlib.h>

TEST(Netlib, Simple)
{
	ape_global * g_ape;

	g_ape = NULL;

	g_ape = native_netlib_init();
	EXPECT_TRUE(g_ape != NULL);
	ape_running = g_ape->is_running = 0;

	//native_netlib_destroy(g_ape);
}

