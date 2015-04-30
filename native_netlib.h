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

#ifndef _NATIVE_NETLIB_H_
#define _NATIVE_NETLIB_H_

#include "common.h"
#include "ape_socket.h"
#include "ape_events_loop.h"
#include "ape_timers.h"
#include "ape_timers_next.h"

#ifdef __cplusplus
extern "C" {
#endif

ape_global *native_netlib_init();
void native_netlib_destroy(ape_global *ape);

#ifdef __cplusplus
}
#endif

#endif
