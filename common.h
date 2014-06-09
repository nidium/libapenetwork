/*
    APE Network Library
    Copyright (C) 2010-2013 Anthony Catel <paraboul@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdlib.h>

#ifndef _APE_COMMON_H_
#define _APE_COMMON_H_

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

#define APE_BASEMEM 4096

#ifndef APE_TIMER_RESOLUTION
  #define APE_TIMER_RESOLUTION 8 /* ms */
#endif

#define ape_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))
#define ape_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))

#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define CONST_STR_LEN2(x) x ? sizeof(x) - 1 : 0, x

#define _APE_ABS_MASK(val) (val >> sizeof(int) * 8 - 1)
#define APE_ABS(val) (val + _APE_ABS_MASK(val)) ^ _APE_ABS_MASK(val)

typedef struct _ape_global ape_global;

#include "ape_events.h"
#include "ape_timers_next.h"
#ifdef _HAVE_SSL_SUPPORT
  #include "ape_ssl.h"
#endif
#include "ape_hash.h"

struct _ape_global {
    int basemem;
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

	struct {
		struct _ticks_callback *timers;
		unsigned int ntimers;
	} timers;

	ape_timers timersng;
	
  int is_running;
};


#endif
