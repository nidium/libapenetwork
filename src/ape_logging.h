/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef _APE_LOGGING_H
#define _APE_LOGGING_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

static const char * ape_log_levellabels[] = {"DEBUG", "WARN", "ERROR", "INFO"};

typedef enum _ape_log_lvl_t {
    APE_LOG_DEBUG = 0,
    APE_LOG_WARN,
    APE_LOG_ERROR,
    APE_LOG_INFO
} ape_log_lvl_t;

typedef void (*ape_log_init_callback_t) (void * args);
typedef void (*ape_log_log_callback_t) (void* cb_args, ape_log_lvl_t lvl, \
    const char * lvl_label, const char * tag, const char * buff);
typedef void (*ape_log_clear_callback_t) (void * args);

typedef struct _ape_logger_t {
    ape_log_lvl_t            lvl;
    ape_log_init_callback_t  init;
    ape_log_log_callback_t   log;
    ape_log_clear_callback_t clear;
    void *                   cb_args;
} ape_logger_t;

void APE_SetLogger(ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const ape_log_init_callback_t init, const ape_log_log_callback_t log, \
    const ape_log_clear_callback_t clear, void * cb_args);
int APE_Logf(const ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const char * tag, const char * fmt, ...);
int APE_Log(const ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const char * tag, const char *buffer);

#define APE_DEBUG(tag, fmt, ...) APE_Logf(logger, APE_LOG_DEBUG, tag, fmt, __VA_ARGS__);
#define APE_WARN(tag, fmt, ...) APE_Logf(logger, APE_LOG_WARN, tag, fmt, __VA_ARGS__);
#define APE_ERROR(tag, fmt, ...) APE_Logf(logger, APE_LOG_ERROR, tag, fmt, __VA_ARGS__);
#define APE_INFO(tag, fmt, ...) APE_Logf(logger, APE_LOG_INFO, tag, fmt, __VA_ARGS__);

#ifdef __cplusplus
}
#endif
#endif /* ape_logging.h */

