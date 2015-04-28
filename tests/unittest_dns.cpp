#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

#include <ares.h>

#include <ape_dns.h>

TEST(DNS, Invalidate)
{
	ape_dns_state state;

	state.invalidate = 2;
	ape_dns_invalidate(&state);
	EXPECT_EQ(state.invalidate, 1);
}

