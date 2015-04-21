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

#ifndef __APE_TIMERS_H
#define __APE_TIMERS_H

#include "common.h"

#define VTICKS_RATE 50 // 50 ms

typedef enum {
	APE_TIMER_PROTECTED = 1 << 0,
	APE_TIMER_RESAMPLE = 1 << 1,
} ape_timer_flags;

struct _ticks_callback
{
	int ticks_need;
	int delta;
	int times;
	unsigned int identifier;
	int flag;

	void *func;
	void *params;
	
	struct _ticks_callback *next;
};

#ifdef __cplusplus
extern "C" {
#endif

void process_tick(ape_global *ape);
struct _ticks_callback *add_timeout(unsigned int msec, void *callback, void *params, ape_global *ape);
struct _ticks_callback *add_periodical(unsigned int msec, int times, void *callback, void *params, int resamp, ape_global *ape);
void del_timer_identifier(unsigned int identifier, ape_global *ape);
struct _ticks_callback *get_timer_identifier(unsigned int identifier, ape_global *ape);
int get_first_timer_ms(ape_global *ape);
void timers_free(ape_global *ape);

#define add_ticked(x, y) add_periodical(VTICKS_RATE, 0, x, y, ape)
#define ape_dispatch_async(callback, params) add_timeout(1, callback, params, ape)

#ifdef __cplusplus
}
#endif
#endif

