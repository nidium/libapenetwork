#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

#include <native_netlib.h>

TEST(Netlib, Simple)
{
	ape_global * g_ape;

	g_ape = NULL;

	g_ape = native_netlib_init();
	native_netlib_destroy(g_ape);
	EXPECT_TRUE(g_ape != NULL);
}

