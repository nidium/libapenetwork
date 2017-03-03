/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef _APE_NETLIB_H_
#define _APE_NETLIB_H_

#include "ape_common.h"
#include "ape_socket.h"
#include "ape_events_loop.h"
#include "ape_timers_next.h"

#ifdef __cplusplus
extern "C" {
#endif

ape_global *APE_init();
void APE_destroy(ape_global *ape);

/* Read back the value from thread local storage */
ape_global *APE_get();

#ifdef __cplusplus
}

#endif

#endif

