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

typedef enum _ape_log_lvl_t {
    APE_LOG_DEBUG   = 0x01,
    APE_LOG_WARN    = 0x02,
    APE_LOG_ERROR   = 0x04,
    APE_LOG_INFO    = 0x08
} ape_log_lvl_t;

typedef void (*ape_log_init_cbt) (void * args);
typedef void (*ape_log_log_cbt) (void* cb_args, ape_log_lvl_t lvl, \
    const char * lvl_label, const char * tag, const char * fmt, va_list args);
typedef void (*ape_log_clear_cbt) (void * args);

typedef struct _ape_logger_t {
    ape_log_lvl_t       lvl;
    ape_log_init_cbt    init;
    ape_log_log_cbt     log;
    ape_log_clear_cbt   clear;
    void *              cb_args;
} ape_logger_t;

void APE_SetLogger(ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const ape_log_init_cbt init, const ape_log_log_cbt log, \
    const ape_log_clear_cbt clear, void * cb_args);
int APE_Log(const ape_logger_t * logger, const ape_log_lvl_t lvl, \
    const char * tag, const char * fmt, ...);

#define APE_DEBUG(tag, fmt, ...) APE_Log(logger, APE_LOG_DEBUG, tag, fmt, __VA_ARGS__);
#define APE_WARN(tag, fmt, ...) APE_Log(logger, APE_LOG_WARN, tag, fmt, __VA_ARGS__);
#define APE_ERROR(tag, fmt, ...) APE_Log(logger, APE_LOG_ERROR, tag, fmt, __VA_ARGS__);
#define APE_INFO(tag, fmt, ...) APE_Log(logger, APE_LOG_INFO, tag, fmt, __VA_ARGS__);

#ifdef __cplusplus
}
#endif
#endif /* ape_logging.h */
