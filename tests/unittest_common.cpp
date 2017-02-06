/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_common.h>

TEST(Common, MinMaxMacro)
{
    int val1, val2, mini, maxi;

    val1 = 10;
    val2 = val1 * 10;
    mini = ape_min(val1, val2);
    maxi = ape_max(val1, val2);
    EXPECT_EQ(mini, val1);
    EXPECT_EQ(maxi, val2);
}

TEST(Common, MinMaxMacroNegPos)
{
    int val1, val2, mini, maxi;

    val1 = -10;
    val2 = val1 * -10;
    mini = ape_min(val1, val2);
    maxi = ape_max(val1, val2);
    EXPECT_EQ(mini, val1);
    EXPECT_EQ(maxi, val2);
}

TEST(Common, MinMaxMacroNegNeg)
{
    int val1, val2, mini, maxi;

    val1 = -10;
    val2 = val1 * 10;
    mini = ape_min(val1, val2);
    maxi = ape_max(val1, val2);
    EXPECT_EQ(mini, val2);
    EXPECT_EQ(maxi, val1);
}

TEST(Common, MinMaxMacroEq)
{
    int val1, val2, mini, maxi;

    val1 = 10;
    val2 = val1;
    mini = ape_min(val1, val2);
    maxi = ape_max(val1, val2);
    EXPECT_EQ(mini, val1);
    EXPECT_EQ(maxi, val2);
    EXPECT_EQ(maxi, mini);
}

TEST(Common, AbsSsizePos)
{
    signed long calculated, target;
    ssize_t test;

    test = target = 8;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsSsizeNeg)
{
    signed long calculated, target;
    ssize_t test;

    test       = -8;
    target     = test * -1;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsCharPos)
{
    signed long calculated, target;
    signed char test;

    test = target = 120;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsShortNeg)
{
    signed long calculated, target;
    signed short test;

    test       = -120;
    target     = test * -1;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsShortPos)
{
    signed long calculated, target;
    signed short test;

    test = target = 32760;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsIntNeg)
{
    signed long calculated, target;
    signed int test;

    test       = -32760;
    target     = test * -1;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsIntPos)
{
    signed long calculated, target;
    signed int test;

    test = target = 2147483640;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsIntMaxNeg)
{
    signed long calculated, target;
    signed int test;

    test       = -2147483640;
    target     = test * -1;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsLongPos)
{
    signed long calculated, target;
    signed long test;

    test = target = 2147483640;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

TEST(Common, AbsLongNeg)
{
    signed long calculated, target;
    signed long test;

    test       = -2147483640;
    target     = test * -1;
    calculated = APE_ABS(test);
    EXPECT_EQ(calculated, target);
}

struct constStrMacro {
    const char *str;
    const int len;
};

TEST(Common, ConstStrLenMacroNull)
{
#define MYSTR NULL
    struct constStrMacro test = { CONST_STR_LEN(MYSTR) };
    EXPECT_EQ(test.len, 0);
    EXPECT_TRUE(test.str == NULL);
#undef MYSTR
}

TEST(Common, ConstStrLenMacroHello)
{
#define MYSTR "HELLO"
    struct constStrMacro test = { CONST_STR_LEN(MYSTR) };
    EXPECT_EQ(test.len, 5);
    EXPECT_TRUE(strcmp(test.str, MYSTR) == 0);
#undef MYSTR
}

TEST(Common, ConstStrLenMacroIncomplete)
{
#define MYSTR "H\0ELLO"
    struct constStrMacro test = { CONST_STR_LEN(MYSTR) };
    EXPECT_EQ(test.len, 6);
    EXPECT_TRUE(strcmp(test.str, "H") == 0);
#undef MYSTR
}

