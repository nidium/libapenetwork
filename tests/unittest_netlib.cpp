/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_netlib.h>

TEST(Netlib, Simple)
{
    ape_global * g_ape;

    g_ape = NULL;

    g_ape = APE_init();
    EXPECT_TRUE(g_ape != NULL);
    EXPECT_EQ(g_ape->is_running, 1);
    EXPECT_EQ(g_ape->timersng.run_in_low_resolution, 0);
    EXPECT_EQ(g_ape->timersng.last_identifier, 0);
    EXPECT_TRUE(g_ape->timersng.head == NULL);
    EXPECT_TRUE(g_ape->ctx == NULL);
    
    ape_running = g_ape->is_running = 0;

    APE_destroy(g_ape);
}

