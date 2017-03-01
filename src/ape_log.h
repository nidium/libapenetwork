/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef _APE_LOG_H
#define _APE_LOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

static const char *ape_log_levellabels[] = {"ERROR", "WARN", "INFO", "DEBUG", ""};


typedef enum _ape_log_lvl_t {
    APE_LOG_ERROR = 0,
    APE_LOG_WARN,
    APE_LOG_INFO,
    APE_LOG_DEBUG,
    APE_LOG_COUNT
} ape_log_lvl_t;

const char *APE_getloglabel(ape_log_lvl_t lvl);

typedef void *(*ape_log_init_callback_t)(void *ctx);
typedef void (*ape_log_log_callback_t)(void *ctx, void *cb_args, ape_log_lvl_t lvl,
                                            const char *tag, const char *buff);
typedef void (*ape_log_cleanup_callback_t)(void *ctx, void *cb_args);

typedef struct _ape_logger_t {
    ape_log_lvl_t            lvl;
    ape_log_init_callback_t  init;
    ape_log_log_callback_t   log;
    ape_log_cleanup_callback_t cleanup;
    void *                   cb_args;
    void *                   ctx;
} ape_logger_t;

void APE_setlogger(const ape_log_lvl_t lvl,
    const ape_log_init_callback_t init, const ape_log_log_callback_t log,
    const ape_log_cleanup_callback_t cleanup, void *ctx);
int APE_logf(const ape_log_lvl_t lvl,
    const char *tag, const char *fmt, ...);
int APE_log(const ape_log_lvl_t lvl,
    const char *tag, const char *buffer);

#if 0
  //tread-local-storage
  #define APE_DEBUG(tag, fmt, ...) APE_logf(logger, APE_LOG_DEBUG, tag, fmt, __VA_ARGS__);
  #define APE_WARN(tag, fmt, ...) APE_logf(logger, APE_LOG_WARN, tag, fmt, __VA_ARGS__);
  #define APE_ERROR(tag, fmt, ...) APE_logf(logger, APE_LOG_ERROR, tag, fmt, __VA_ARGS__);
  #define APE_INFO(tag, fmt, ...) APE_logf(logger, APE_LOG_INFO, tag, fmt, __VA_ARGS__);
#else
  #define APE_DEBUG(tag, fmt, ...) /*pass*/
  #define APE_WARN(tag, fmt, ...)  /*pass*/
  #define APE_ERROR(tag, fmt, ...) /*pass*/
  #define APE_INFO(tag, fmt, ...)  /*pass*/

#endif

#ifdef __cplusplus
}
#endif
#endif /* ape_log.h */

