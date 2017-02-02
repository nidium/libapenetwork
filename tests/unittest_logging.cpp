/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_netlib.h>
#include <ape_logging.h>

struct args {
    FILE *file_h;
};


static void loglog(void * cb_args, ape_log_lvl_t lvl, const char * lvl_label, const char * tag,
    const char * buffer )
{
    struct args * args;

    args = (struct args*) cb_args;
    if (args->file_h) {
        int datelen;
        char date[32];
        time_t log_ts;

        log_ts = time(NULL);
        datelen = strftime(&date[0], 32, "%Y-%m-%d %H:%M:%S - ", localtime(&log_ts));
        fprintf(args->file_h, "%s\t%s\n", date, buffer);
    }
}

static void loginit(void * cb_args)
{
    struct args * args;
    const char * tmp_file = "/tmp/test_ape_logging.log";

    args = (struct args*) cb_args;
    if (! args->file_h) {
        args->file_h = fopen(tmp_file, "wb");
    }
    fprintf(args->file_h, "writing stuff\n");
}

static void logclear (void * cb_args)
{
    struct args * args;

    args = (struct args*) cb_args;
    if (args->file_h) {
        fprintf(args->file_h, "clearing log\n");
        fflush(args->file_h);
        fclose(args->file_h);
    }
}

TEST(Logger,  Logger)
{
    ape_global *g_ape;
    g_ape = APE_init();
    ape_logger_t logger;
    struct args args;
    int fwd;

    args.file_h = NULL;
    memset(&logger, 0, sizeof(logger));
    APE_SetLogger(&logger, APE_LOG_ERROR, loginit, loglog, logclear, &args);

    EXPECT_TRUE(logger.lvl == APE_LOG_ERROR);
    EXPECT_TRUE(logger.init == loginit);
    EXPECT_TRUE(logger.log == loglog);
    EXPECT_TRUE(logger.clear == logclear);
    EXPECT_TRUE(logger.cb_args == &args);

    fwd = APE_Logf(&logger, APE_LOG_ERROR, "tag", "should %s print", "indeed");
    EXPECT_TRUE(fwd == 1);
    fwd = APE_Logf(&logger, APE_LOG_INFO, "tag", "should %s print", "not");
    EXPECT_TRUE(fwd == 0);
    fwd = APE_Log(&logger, APE_LOG_ERROR, "tag", "should print");
    EXPECT_TRUE(fwd == 1);
    fwd = APE_Log(&logger, APE_LOG_INFO, "tag", "should not print");
    EXPECT_TRUE(fwd == 0);

    logger.clear(logger.cb_args);
}

