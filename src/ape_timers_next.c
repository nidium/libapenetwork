/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_timers_next.h"
#include "ape_common.h"
#include <stdlib.h>
#include <stdio.h>

static void process_async(ape_timers *timers);
static void del_async_all(ape_timers *timers);
static ape_timer_t *ape_timer_destroy(ape_timers *timers, ape_timer_t *timer);

struct _ape_timer_t {
    uint64_t identifier;
    int flags;
    uint64_t ticks_needs;
    uint64_t schedule;
    int nexec;
    APE_timer_callback_t callback;
    APE_timer_callback_t clearfunc;
    void *arg;

    struct {
        unsigned int nexec;
        unsigned int max;
        unsigned int min;
        unsigned int totaltime;
    } stats;

    struct _ape_timer_t *next;
    struct _ape_timer_t *prev;
};

struct _ape_timer_async_t {
    APE_timer_callback_t callback;
    APE_timer_callback_t clearfunc;

    void *arg;
    struct _ape_timer_async_t *next;
};

#if defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>

#ifdef __WIN32
LARGE_INTEGER
getFILETIMEoffset()
{
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear         = 1970;
    s.wMonth        = 1;
    s.wDay          = 1;
    s.wHour         = 0;
    s.wMinute       = 0;
    s.wSecond       = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

int clock_gettime(int X, struct timeval *tv)
{
    LARGE_INTEGER t;
    FILETIME f;
    double microseconds;
    static LARGE_INTEGER offset;
    static double frequencyToMicroseconds;
    static int initialized            = 0;
    static BOOL usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter
            = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds
                = (double)performanceFrequency.QuadPart / 1000000.;
        } else {
            offset                  = getFILETIMEoffset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter)
        QueryPerformanceCounter(&t);
    else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart   = microseconds;
    tv->tv_sec   = t.QuadPart / 1000000;
    tv->tv_usec = t.QuadPart % 1000000;
    return (0);
}
static __inline uint64_t mach_absolute_time()
{
    struct timeval t;
    clock_gettime(0, &t);

    return ((uint64_t)t.tv_sec * 1000000 + (uint64_t)t.tv_usec) * 1000;
}
#else // !__WIN32
static __inline uint64_t mach_absolute_time()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);

    return (uint64_t)t.tv_sec * 1000000000 + (uint64_t)t.tv_nsec;
}
#endif
#endif

int ape_timers_process(ape_global *ape_ctx)
{
    ape_timers *timers = &ape_ctx->timersng;

    ape_timer_t *cur = timers->head;
    uint64_t inums = UINT64_MAX, lastsample = 0;

    process_async(timers);

    /* TODO: paused timer */
    while (cur != NULL) {
        uint64_t start;

        if (cur->flags & APE_TIMER_IS_CLEARED) {
            cur = ape_timer_destroy(timers, cur);
            continue;
        }

        if ((start = mach_absolute_time()) >= cur->schedule - 150000) {
            uint64_t ret;
            unsigned int duration;

            ret = cur->callback(cur->arg);

            // printf("Timer returned %lld\n", ret);

            if (ret == -1) {
                cur->schedule = start + cur->ticks_needs;
            } else if (ret == 0) {
                cur = ape_timer_destroy(timers, cur);
                continue;
            } else {
                cur->ticks_needs = ret * 1000000;
                cur->schedule    = start + cur->ticks_needs;
            }

            lastsample = mach_absolute_time();
            duration   = lastsample - start;

            if (cur->stats.max < duration / 1000000) {
                cur->stats.max = duration / 1000000;
            }
            if (cur->stats.min == 0 || duration / 1000000 < cur->stats.min) {
                cur->stats.min = duration / 1000000;
            }
            cur->stats.nexec++;
            cur->stats.totaltime += duration / 1000000;
        }

        if (cur->schedule < inums) {
            inums = cur->schedule;
        }

        cur = cur->next;
    }

    process_async(timers);

    if (inums == UINT64_MAX) {
        return APE_TIMER_RESOLUTION;
    }

    if (lastsample == 0) {
        lastsample = mach_absolute_time();
    }

    // printf("Next timer in : %lld or %d\n", inums-lastsample,  ape_max(1,
    // (int)((inums-lastsample+500000)/1000000)));

    return ape_max((timers->run_in_low_resolution ? 100 : 1),
                   (int)((inums - lastsample + 500000) / 1000000));
}

int APE_timer_getflags(ape_timer_t *timer)
{
    return timer->flags;
}

void APE_timer_setflags(ape_timer_t *timer, int flags)
{
    timer->flags = flags;
}

void APE_timer_unprotect(ape_timer_t *timer)
{
    timer->flags &= ~APE_TIMER_IS_PROTECTED;
}

void APE_timer_setclearfunc(ape_timer_t *timer, APE_timer_callback_t cb)
{
    timer->clearfunc = cb;
}

void APE_async_setclearfunc(ape_timer_async_t *async, APE_timer_callback_t cb)
{
    async->clearfunc = cb;
}

static void process_async(ape_timers *timers)
{
    ape_timer_async_t *cur = timers->head_async;

    /*
        Don't put this after the loop in the
        case new async are created within the loop
    */
    timers->head_async = NULL;

    while (cur != NULL) {
        ape_timer_async_t *ctmp = cur->next;

        cur->callback(cur->arg);
        cur->clearfunc(cur->arg);

        free(cur);
        cur = ctmp;
    }
}

ape_timer_t *APE_timer_getbyid(ape_global *ape_ctx, uint64_t identifier)
{
    ape_timer_t *cur;
    ape_timers *timers = &ape_ctx->timersng;

    for (cur = timers->head; cur != NULL; cur = cur->next) {
        if (cur->identifier == identifier) {
            return cur;
        }
    }
    return NULL;
}

void *APE_timer_getarg(ape_timer_t *timer)
{
    return timer->arg;
}

uint64_t APE_timer_getid(ape_timer_t *timer)
{
    return timer->identifier;
}

void APE_timer_clearbyid(ape_global *ape_ctx, uint64_t identifier, int force)
{
    ape_timer_t *cur;
    ape_timers *timers = &ape_ctx->timersng;

    for (cur = timers->head; cur != NULL; cur = cur->next) {
        if (cur->identifier == identifier) {
            if (!(cur->flags & APE_TIMER_IS_PROTECTED)
                || ((cur->flags & APE_TIMER_IS_PROTECTED) && force)) {

                cur->flags |= APE_TIMER_IS_CLEARED;
            }
            return;
        }
    }
}

void APE_timers_destroy_unprotected(ape_global *ape_ctx)
{
    ape_timers *timers = &ape_ctx->timersng;
    ape_timer_t *cur   = timers->head;

    while (cur != NULL) {
        if (!(cur->flags & APE_TIMER_IS_PROTECTED)) {
            cur = ape_timer_destroy(timers, cur);
            continue;
        }

        cur = cur->next;
    }

    del_async_all(timers);
}

void APE_timers_destroy_all(ape_global *ape_ctx)
{
    ape_timers *timers = &ape_ctx->timersng;
    ape_timer_t *cur   = timers->head;

    while (cur != NULL) {
        cur = ape_timer_destroy(timers, cur);
    }

    del_async_all(timers);
}

static void del_async_all(ape_timers *timers)
{
    ape_timer_async_t *async = timers->head_async;

    timers->head_async = NULL;

    while (async != NULL) {
        ape_timer_async_t *tmp = async->next;
        async->clearfunc(async->arg);
        free(async);

        async = tmp;
    }
}

/* delete *timer* and returns *timer->next* */
static ape_timer_t *ape_timer_destroy(ape_timers *timers, ape_timer_t *timer)
{
    ape_timer_t *ret;

    if (timer->prev == NULL) {
        timers->head = timer->next;
    } else {
        timer->prev->next = timer->next;
    }
    if (timer->next != NULL) {
        timer->next->prev = timer->prev;
    }
    ret = timer->next;

    if (timer->clearfunc) {
        timer->clearfunc(timer->arg);
    }

    free(timer);

    return ret;
}

void APE_timer_destroy(ape_global *ape_ctx, ape_timer_t *timer)
{
    ape_timers *timers = &ape_ctx->timersng;

    (void)ape_timer_destroy(timers, timer);
}

void ape_timer_stats_print(ape_timer_t *timer)
{
    printf("%zd\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", timer->identifier,
           timer->stats.nexec, timer->stats.totaltime, timer->stats.max,
           timer->stats.min,
           (timer->stats.nexec == 0
                ? timer->stats.totaltime
                : timer->stats.totaltime / timer->stats.nexec));
}

void ape_timers_stats_print(ape_global *ape_ctx)
{
    ape_timer_t *cur;
    ape_timers *timers = &ape_ctx->timersng;

    printf("=======================\n");
    printf("Id\t\ttimes\texec\t\tmax\t\tmin\t\tavg\n");
    for (cur = timers->head; cur != NULL; cur = cur->next) {
        ape_timer_stats_print(cur);
    }
    printf("=======================\n");
}

ape_timer_async_t *APE_async(ape_global *ape_ctx, APE_timer_callback_t cb,
                             void *arg)
{
    ape_timer_async_t *async
        = (ape_timer_async_t *)malloc(sizeof(ape_timer_async_t));
    ape_timers *timers = &ape_ctx->timersng;

    async->callback  = cb;
    async->clearfunc = NULL;
    async->next      = timers->head_async;
    async->arg       = arg;

    timers->head_async = async;

    return async;
}

ape_timer_t *APE_timer_create(ape_global *ape_ctx, int ms,
                              APE_timer_callback_t cb, void *arg)
{
    ape_timers *timers = &ape_ctx->timersng;

    if (cb == NULL || timers == NULL) {
        return NULL;
    }
    ape_timer_t *timer = (ape_timer_t *)malloc(sizeof(ape_timer_t));
    timers->last_identifier++;
    timer->callback    = cb;
    timer->ticks_needs = (uint64_t)ms * 1000000LL;
    timer->schedule    = mach_absolute_time() + timer->ticks_needs;
    timer->arg         = arg;
    timer->flags       = APE_TIMER_IS_PROTECTED;
    timer->prev        = NULL;
    timer->identifier  = timers->last_identifier;
    timer->next        = timers->head;
    timer->clearfunc   = NULL;

    timer->stats.nexec     = 0;
    timer->stats.totaltime = 0;
    timer->stats.max       = 0;
    timer->stats.min       = 0;

    if (timers->head != NULL) {
        timers->head->prev = timer;
    }

    timers->head = timer;
    // printf("Timer added %d %p\n", ms, timers->head);
    return timer;
}

void APE_timer_setlowresolution(ape_global *ape_ctx, int low)
{
    ape_timers *timers = &ape_ctx->timersng;

    timers->run_in_low_resolution = low;
}

