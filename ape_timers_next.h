/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_TIMERS_NEXT_H
#define __APE_TIMERS_NEXT_H

#include <stdint.h>

typedef int (*timer_callback)(void *arg);

enum {
	APE_TIMER_IS_PROTECTED = 1 << 0,
	APE_TIMER_IS_CLEARED   = 1 << 1
};

typedef struct _ape_timer
{
	uint64_t identifier;
	int flags;
	uint64_t ticks_needs;
	uint64_t schedule;
	int nexec;
	timer_callback callback;
	timer_callback clearfunc;
	void *arg;

	struct {
		unsigned int nexec;
		unsigned int max;
		unsigned int min;
		unsigned int totaltime;
	} stats;

	struct _ape_timer *next;
	struct _ape_timer *prev;
} ape_timer;

typedef struct _ape_async
{
    timer_callback callback;
    timer_callback clearfunc;

    void *arg;
    struct _ape_async *next;
} ape_async;

typedef struct _ape_timers
{
	ape_timer *head;
    ape_async *head_async;

	uint64_t last_identifier;
    int run_in_low_resolution;
} ape_timers;

#ifdef __cplusplus
extern "C" {
#endif

int process_timers(ape_timers *timers);
ape_timer *del_timer(ape_timers *timers, ape_timer *timer);
ape_timer *add_timer(ape_timers *timers, int ms, timer_callback cb, void *arg);
ape_timer *get_timer_by_id(ape_timers *timers, uint64_t identifier);
void clear_timer_by_id(ape_timers *timers, uint64_t identifier, int force);
void timer_stats_print(ape_timer *timer);
void timers_stats_print(ape_timers *timers);
void del_timers_unprotected(ape_timers *timers);
void del_timers_all(ape_timers *timers);
void set_timer_to_low_resolution(ape_timers *timers, int low);

ape_async *add_async(ape_timers *timers, timer_callback cb, void *arg);

#ifdef __cplusplus
}
#endif

#define timer_dispatch_async(callback, params) add_timer(&ape->timersng, 0, callback, params)
#define timer_dispatch_async_unprotected(callback, params) add_timer(&ape->timersng, 0, callback, params)->flags &= ~APE_TIMER_IS_PROTECTED


#endif

