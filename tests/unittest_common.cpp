#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <common.h>

TEST(Common, MinMaxMacro)
{
	int val1, val2, mini, maxi;

	val1 = 10;
	val2 = val1 *10;
	mini = ape_min(val1, val2);
	maxi = ape_max(val1, val2);
	EXPECT_EQ(mini, val1);
	EXPECT_EQ(maxi, val2);
}

TEST(Common, ABS)
{
	signed long calculated, target;

	{
	ssize_t test;
	
	test = target = 8;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	
	test = -8;
	target = test * -1;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	}
	
	{
	signed char test;
	
	test = target = 120;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	
	test = -120;
	target = test * -1;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	}

	{
	signed short test;
	
	test = target = 32760;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	
	test = -32760;
	target = test * -1;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	}

	{
	signed int test;
	
	test = target = 2147483640;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	
	test = -2147483640;
	target = test * -1;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	}

	{
	signed long test;
	
	test = target = 2147483640;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	
	test = -2147483640;
	target = test * -1;
	calculated = APE_ABS(test);
	EXPECT_EQ(calculated, target);
	}

}
struct constStrMacro{
	const char * str;
	const int len;
};

TEST(Common, ConstStrLenMacro)
{
	{
#define MYSTR NULL
	struct constStrMacro test = { CONST_STR_LEN(MYSTR)};
	EXPECT_EQ(test.len, 0);
	EXPECT_TRUE(test.str == NULL);
#undef MYSTR
	}
	{
#define MYSTR "HELLO"
	struct constStrMacro test = { CONST_STR_LEN(MYSTR)};
	EXPECT_EQ(test.len, 5);
	EXPECT_TRUE(strcmp(test.str, MYSTR) == 0 );
#undef MYSTR
	}
	{
#define MYSTR "H\0ELLO"
	struct constStrMacro test = { CONST_STR_LEN(MYSTR)};
	EXPECT_EQ(test.len, 6);
	EXPECT_TRUE(strcmp(test.str, "H") == 0 );
#undef MYSTR
	}

}

