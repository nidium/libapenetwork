/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_log.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void APE_setlogger(ape_logger_t *logger, const ape_log_lvl_t lvl,
    const ape_log_init_callback_t init, const ape_log_log_callback_t log,
    const ape_log_cleanup_callback_t cleanup, void *ctx)
{
    if (logger->cleanup) {
        logger->cleanup(logger->ctx, logger->cb_args);
    }

    logger->lvl = lvl;
    logger->init = init;
    logger->log = log;
    logger->cleanup = cleanup;
    logger->ctx = ctx;

    if (logger->init) {
        logger->cb_args = logger->init(ctx);
    }
}

const char *APE_getloglabel(ape_log_lvl_t lvl)
{
    return ape_log_levellabels[lvl];
}

int APE_log(const ape_logger_t *logger, const ape_log_lvl_t lvl,
    const char *tag, const char *buffer)
{
    if (logger->log && logger->lvl >= lvl) {
        logger->log(logger->ctx, logger->cb_args, lvl, tag, buffer);

        return 1;
    }

    return 0;
}

int APE_logf(const ape_logger_t *logger, const ape_log_lvl_t lvl,
    const char *tag, const char * fmt, ...)
{
    int logged;
    va_list args;
    char* buff;

    va_start(args, fmt);
    vasprintf(&buff, fmt, args);
    va_end(args);

    logged = APE_log(logger, lvl, tag, buff);

    free(buff);

    return logged;
}

