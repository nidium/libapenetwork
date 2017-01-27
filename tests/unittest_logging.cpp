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

static void loglog(void * cb_args, ape_log_lvl_t lvl, const char * lvl_label, const char * tag,
    const char * fmt, va_list args)
{
    FILE * file_h;

    file_h = (FILE*) cb_args;
    if (file_h) {
        int datelen;
        char date[32];
        time_t log_ts;

        log_ts = time(NULL);
        datelen = strftime(&date[0], 32, "%Y-%m-%d %H:%M:%S - ", localtime(&log_ts));
        fprintf(file_h, "%s\t", date);
        fprintf(file_h, fmt, args);
        fprintf(file_h, "\n");
    }
}

static void loginit(void * cb_args)
{
    FILE * file_h;
    const char * tmp_file = "/tmp/test_ape_logging.log";

    file_h = (FILE*) cb_args;
    if (! file_h) {
        file_h = fopen(tmp_file, "wb");
    }
    fprintf(file_h, "writing stuff\n");
}

static void logclear (void * cb_args)
{
    FILE * file_h;

    file_h = (FILE*) cb_args;
    if (file_h) {
        fclose(file_h);
    }
}

TEST(Logger,  Logger)
{
    ape_global *g_ape;
    g_ape = APE_init();
    ape_logger_t logger;
    FILE *file_h;
    int fwd;

    file_h = NULL;
    memset(&logger, 0, sizeof(logger));
    APE_SetLogger(&logger, APE_Log_error, loginit, loglog, logclear, file_h);

    EXPECT_TRUE(logger.lvl == APE_Log_error);
    EXPECT_TRUE(logger.init == loginit);
    EXPECT_TRUE(logger.log == loglog);
    EXPECT_TRUE(logger.clear == logclear);
    EXPECT_TRUE(logger.cb_args == file_h);

    fwd = APE_Log(&logger, APE_Log_error, "tag," "should %s print", "");
    EXPECT_TRUE(fwd == 1);
    fwd = APE_Log(&logger, APE_Log_info, "tag," "should %s print", "not");
    EXPECT_TRUE(fwd == 0);

    logger.clear(logger.cb_args);
}

