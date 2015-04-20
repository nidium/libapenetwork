#include <stdlib.h>
#include <string.h>

#include <gtest/gtest.h>

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

#if 0
static void strLenTest( const char * orgString, const int shouldLen, const char * macroString, const int macroLen )
{
	EXPECT_EQ(shouldLen, macroLen);
	EXPECT_TRUE(strcmp(orgString,macroString) == 0);

}

TEST(Common, ConstStrLenMacro)
{
	char * str;
	size_t len;
	const char fixed[16] = "HELLO";

	str = strdup("HELLO"); len = 5;
	strLenTest( str, len, CONST_STR_LEN(str));
	free(str);

	str = strdup("NULL"); len = 0;
	strLenTest( str, len, CONST_STR_LEN(str));
	free(str);

	str = strdup("\0HELLO"); len = 0;
	strLenTest( str, len, CONST_STR_LEN(str));
	free(str);

	str = strdup("H\0ELLO"); len = 1;
	strLenTest( str, len, CONST_STR_LEN(str));
	free(str);

	str = strdup(fixed); len = 5;
	strLenTest( str, len, CONST_STR_LEN(str));
	free(str);
}

#endif
