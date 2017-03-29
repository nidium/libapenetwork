/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "unittest.h"

#include <ape_netlib.h>
#include <ape_log.h>


static void loglog(void *ctx, void *cb_args, ape_log_lvl_t lvl, const char * tag,
    const char * buffer )
{
    FILE *file_h = (FILE *)cb_args;
    if (file_h) {
        int datelen;
        char date[32];
        time_t log_ts;

        log_ts = time(NULL);
        datelen = strftime(&date[0], 32, "%Y-%m-%d %H:%M:%S - ", localtime(&log_ts));
        fprintf(file_h, "%s\t%s\n", date, buffer);
    }
}

static void *loginit(void *ctx)
{
    const char * tmp_file = "/tmp/test_ape_logging.log";

    FILE *file_h = fopen(tmp_file, "wb");

    fprintf(file_h, "writing stuff\n");

    return file_h;
}

static void logcleanup (void *ctx, void * cb_args)
{
    FILE *file_h = (FILE *)cb_args;

    if (file_h) {
        fprintf(file_h, "clearing log\n");
        fflush(file_h);
        fclose(file_h);
    }
}

TEST(Logger,  Logger)
{
    ape_global *g_ape;
    g_ape = APE_init();
    int fwd;

    APE_setlogger(APE_LOG_ERROR, loginit, loglog, logcleanup, NULL);

    fwd = APE_logf(APE_LOG_ERROR, "tag", "should %s print", "indeed");
    EXPECT_TRUE(fwd == 1);
    fwd = APE_logf(APE_LOG_INFO, "tag", "should %s print", "not");
    EXPECT_TRUE(fwd == 0);
    fwd = APE_log(APE_LOG_ERROR, "tag", "should print");
    EXPECT_TRUE(fwd == 1);
    fwd = APE_log(APE_LOG_INFO, "tag", "should not print");
    EXPECT_TRUE(fwd == 0);
    if (g_ape->logger.cleanup) {
        g_ape->logger.cleanup(g_ape->logger.ctx, g_ape->logger.cb_args);
    }
}

