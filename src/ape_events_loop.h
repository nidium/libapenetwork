/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_EVENTS_LOOP_H_
#define __APE_EVENTS_LOOP_H_

#include "ape_common.h"

#ifdef __cplusplus
extern "C" {
#endif


void APE_loop_run(ape_global *ape);
void APE_loop_stop();

#ifdef __cplusplus
}
#endif

#endif

