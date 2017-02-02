/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ape_logging.h"

void APE_SetLogger(ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const ape_log_init_callback_t init, const ape_log_log_callback_t log,
    const ape_log_clear_callback_t clear, void * cb_args)
{
    if (logger->clear) {
        logger->clear(logger->cb_args);
        logger->cb_args = NULL;
    }
    logger->lvl = lvl;
    logger->init = init;
    logger->log = log;
    logger->clear = clear;
    logger->cb_args = cb_args;
    if (logger->init) {
        logger->init(logger->cb_args);
    }
}

int APE_Log(const ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const char * tag, const char *buffer)
{
    const char* lvl_label;
    if (logger->log && logger->lvl >= lvl) {
        lvl_label = ape_log_levellabels[lvl];
        logger->log(logger->cb_args, lvl, lvl_label, tag, buffer);

        return 1;
    }

    return 0;
}

int APE_Logf(const ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const char * tag, const char * fmt, ...)
{
    int logged;
    va_list args;
    char* buff;

    va_start(args, fmt);
    vasprintf(&buff, fmt, args);

    logged = APE_Log(logger, lvl, tag, buff);

    free(buff);
    va_end(args);

    return logged;
}

