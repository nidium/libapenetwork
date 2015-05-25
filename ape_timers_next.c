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

#include "ape_timers_next.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>

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

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

int
clock_gettime(int X, struct timeval *tv)
{
    LARGE_INTEGER           t;
    FILETIME            f;
    double                  microseconds;
    static LARGE_INTEGER    offset;
    static double           frequencyToMicroseconds;
    static int              initialized = 0;
    static BOOL             usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        } else {
            offset = getFILETIMEoffset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter) QueryPerformanceCounter(&t);
    else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = microseconds;
    tv->tv_sec = t.QuadPart / 1000000;
    tv->tv_usec = t.QuadPart % 1000000;
    return (0);
}
static __inline uint64_t mach_absolute_time()
{
    struct timeval t;
    clock_gettime(0, &t);

    return ((uint64_t)t.tv_sec * 1000000 + (uint64_t)t.tv_usec)*1000;
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

int process_timers(ape_timers *timers)
{
    ape_timer *cur = timers->head;
    uint64_t inums = UINT64_MAX, lastsample = 0;

    /* TODO: paused timer */
    while (cur != NULL) {
        uint64_t start;

        if (cur->flags & APE_TIMER_IS_CLEARED) {
            cur = del_timer(timers, cur);
            continue;
        }

        if ((start = mach_absolute_time()) >= cur->schedule-150000) {
            uint64_t ret;
            unsigned int duration;

            ret = cur->callback(cur->arg);

            //printf("Timer returned %lld\n", ret);

            if (ret == -1) {
                cur->schedule = start + cur->ticks_needs;
            } else if (ret == 0) {
                cur = del_timer(timers, cur);
                continue;
            } else {
                cur->ticks_needs = ret * 1000000;
                cur->schedule = start + cur->ticks_needs;
            }

            lastsample = mach_absolute_time();
            duration = lastsample - start;

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

    if (inums == UINT64_MAX) {
        return APE_TIMER_RESOLUTION;
    }

    if (lastsample == 0) {
        lastsample = mach_absolute_time();
    }

    //printf("Next timer in : %lld or %d\n", inums-lastsample,  ape_max(1, (int)((inums-lastsample+500000)/1000000)));

    return ape_max((timers->run_in_low_resolution ? 100 : 1), (int)((inums-lastsample+500000)/1000000));
}

ape_timer *get_timer_by_id(ape_timers *timers, uint64_t identifier)
{
    ape_timer *cur;
    for (cur = timers->head; cur != NULL; cur = cur->next) {
        if (cur->identifier == identifier) {
            return cur;
        }
    }
    return NULL;
}

void clear_timer_by_id(ape_timers *timers, uint64_t identifier, int force)
{
    ape_timer *cur;
    for (cur = timers->head; cur != NULL; cur = cur->next) {
        if (cur->identifier == identifier) {
            if (!(cur->flags & APE_TIMER_IS_PROTECTED) ||
                ((cur->flags & APE_TIMER_IS_PROTECTED) && force)) {

                cur->flags |= APE_TIMER_IS_CLEARED;
            }
            return;
        }
    }
}

void del_timers_unprotected(ape_timers *timers)
{
    ape_timer *cur = timers->head;

    while (cur != NULL) {
        if (!(cur->flags & APE_TIMER_IS_PROTECTED)) {
            cur = del_timer(timers, cur);
            continue;
        }

        cur = cur->next;
    }
}

void del_timers_all(ape_timers *timers)
{
    ape_timer *cur = timers->head;

    while (cur != NULL) {
        cur = del_timer(timers, cur);
    }
}

/* delete *timer* and returns *timer->next* */
ape_timer *del_timer(ape_timers *timers, ape_timer *timer)
{
    ape_timer *ret;

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

void timer_stats_print(ape_timer *timer)
{
    printf("%zd\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", timer->identifier,
        timer->stats.nexec,
        timer->stats.totaltime,
        timer->stats.max,
        timer->stats.min,
        (timer->stats.nexec == 0 ? timer->stats.totaltime :
            timer->stats.totaltime/timer->stats.nexec));
}

void timers_stats_print(ape_timers *timers)
{
    ape_timer *cur;
    printf("=======================\n");
    printf("Id\t\ttimes\texec\t\tmax\t\tmin\t\tavg\n");
    for (cur = timers->head; cur != NULL; cur = cur->next) {
        timer_stats_print(cur);
    }
    printf("=======================\n");
}

ape_timer *add_timer(ape_timers *timers, int ms, timer_callback cb, void *arg)
{
    if (cb == NULL || timers == NULL) {
        return NULL;
    }
    ape_timer *timer = (ape_timer *)malloc(sizeof(ape_timer));
    timers->last_identifier++;
    timer->callback = cb;
    timer->ticks_needs = (uint64_t)ms * 1000000LL;
    timer->schedule = mach_absolute_time() + timer->ticks_needs;
    timer->arg = arg;
    timer->flags = APE_TIMER_IS_PROTECTED;
    timer->prev = NULL;
    timer->identifier = timers->last_identifier;
    timer->next = timers->head;
    timer->clearfunc = NULL;

    timer->stats.nexec = 0;
    timer->stats.totaltime = 0;
    timer->stats.max = 0;
    timer->stats.min = 0;

    if (timers->head != NULL) {
        timers->head->prev = timer;
    }

    timers->head = timer;
    //printf("Timer added %d %p\n", ms, timers->head);
    return timer;
}

void set_timer_to_low_resolution(ape_timers *timers, int low)
{
    timers->run_in_low_resolution = low;
}

