/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>

#ifndef _APE_COMMON_H_
#define _APE_COMMON_H_

#ifndef USE_SPECIFIC_HANDLER
#ifdef __linux__
#define USE_EPOLL_HANDLER
#elif defined(__APPLE__)
#define USE_KQUEUE_HANDLER
#elif defined(_MSC_VER)
#define USE_SELECT_HANDLER
#define __WIN32
#else
#error "No suitable IO handler found"
#endif
#endif

#ifdef _WIN32
#include "port/windows.h"
#include <winsock2.h>
#else
#include "port/POSIX.h"
#endif

#ifndef APE_TIMER_RESOLUTION
#define APE_TIMER_RESOLUTION 8 /* ms */
#endif

#define ape_min(val1, val2) ((val1 > val2) ? (val2) : (val1))
#define ape_max(val1, val2) ((val1 < val2) ? (val2) : (val1))

#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define CONST_STR_LEN2(x) x ? sizeof(x) - 1 : 0, x

#define _APE_ABS_MASK(val) (val >> (sizeof(int) * 8 - 1))
#define APE_ABS(val) (val + _APE_ABS_MASK(val)) ^ _APE_ABS_MASK(val)

typedef struct _ape_global ape_global;

#include "ape_events.h"
#include "ape_timers_next.h"
#ifdef _HAVE_SSL_SUPPORT
#include "ape_ssl.h"
#endif
#include "ape_hash.h"
#include "ape_logging.h"

struct _ape_global {
    void *ctx; /* public */
#ifdef _HAVE_SSL_SUPPORT
    ape_ssl_t *ssl_global_ctx;
#endif
    unsigned int seed;
    struct _fdevent events;
    struct {
        struct ares_channeldata *channel;
        struct {
            struct _ares_sockets *list;
            size_t size;
            size_t used;
        } sockets;
    } dns;

    ape_timers timersng;
    int is_running;

    uint32_t failed_write_count;
    uint64_t total_memory_buffered;

    int (*kill_handler)(int code, struct _ape_global *ape);

    int urandom_fd;
    ape_logger_t logger;
};

#endif

