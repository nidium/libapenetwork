/*
Copyright 2016 Nidium Inc. All rights reserved.
Use of this source code is governed by a MIT license
that can be found in the LICENSE file.
*/

#ifndef _NATIVE_NETLIB_H_
#define _NATIVE_NETLIB_H_

#include "common.h"
#include "ape_socket.h"
#include "ape_events_loop.h"
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

