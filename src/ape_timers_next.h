/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_TIMERS_NEXT_H
#define __APE_TIMERS_NEXT_H

#include <stdint.h>

typedef struct _ape_global ape_global;
typedef int (*APE_timer_callback_t)(void *arg);
typedef struct _ape_timer_t ape_timer_t;
typedef struct _ape_timer_async_t ape_timer_async_t;

enum { APE_TIMER_IS_PROTECTED = 1 << 0, APE_TIMER_IS_CLEARED = 1 << 1 };


typedef struct _ape_timers {
    ape_timer_t *head;
    ape_timer_async_t *head_async;

    uint64_t last_identifier;
    int run_in_low_resolution;
} ape_timers;

#ifdef __cplusplus
extern "C" {
#endif


ape_timer_t *APE_timer_create(ape_global *ape_ctx, int ms,
                              APE_timer_callback_t cb, void *arg);
ape_timer_t *APE_timer_getbyid(ape_global *ape_ctx, uint64_t identifier);

uint64_t APE_timer_getid(ape_timer_t *timer);

void APE_timer_destroy(ape_global *ape_ctx, ape_timer_t *timer);
void APE_timers_destroy_unprotected(ape_global *ape_ctx);
void APE_timers_destroy_all(ape_global *ape_ctx);
void APE_timer_clearbyid(ape_global *ape_ctx, uint64_t identifier, int force);
void APE_timer_setlowresolution(ape_global *ape_ctx, int low);
void APE_timer_setflags(ape_timer_t *timer, int flags);
int APE_timer_getflags(ape_timer_t *timer);
void APE_timer_unprotect(ape_timer_t *timer);
void APE_timer_setclearfunc(ape_timer_t *timer, APE_timer_callback_t cb);

void *APE_timer_getarg(ape_timer_t *timer);

ape_timer_async_t *APE_async(ape_global *ape_ctx, APE_timer_callback_t cb,
                             void *arg);
void APE_async_setclearfunc(ape_timer_async_t *async, APE_timer_callback_t cb);

int ape_timers_process(ape_global *ape_ctx);
void ape_timer_stats_print(ape_timer_t *timer);
void ape_timers_stats_print(ape_global *ape_ctx);

#ifdef __cplusplus
}
#endif

#define timer_dispatch_async(callback, params)                                 \
    APE_timer_create(ape, 0, callback, params)
#define timer_dispatch_async_unprotected(callback, params)                     \
    APE_timer_unprotect(APE_timer_create(ape, 0, callback, params))

#endif

