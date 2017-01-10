/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <string.h>
#include <stdio.h>

#include "ape_logging.h"


void APE_SetLogger(ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const ape_log_init_cbt init, const ape_log_log_cbt log,
    const ape_log_clear_cbt clear, void * cb_args)
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


// keep these in sync wit ape_logging.h: _ape_log_lvl_t
static const char * ape_log_levellabels[] = {"DEBUG", "WARN", "ERROR", "INFO"};

int APE_Log(const ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const char * tag, const char * fmt, ...)
{
        int fwd = 0;

        if (logger->log && logger->lvl >= lvl) {
            va_list args;
            const char* lvl_label;

            switch (lvl) {
                case APE_Log_info:
                    lvl_label = ape_log_levellabels[3];
                    break;
                case APE_Log_error:
                    lvl_label = ape_log_levellabels[2];
                    break;
                case APE_Log_warn:
                    lvl_label = ape_log_levellabels[1];
                    break;
                case APE_Log_debug: //ft
                default:
                    lvl_label = ape_log_levellabels[0];
                    break;
            }
            va_start(args, fmt);
            logger->log(logger->cb_args, lvl, lvl_label, tag, fmt, args);
            va_end(args);
            fwd = 1;
        }
    return fwd;
}

