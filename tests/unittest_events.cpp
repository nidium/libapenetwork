/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <native_netlib.h>

//@TODO: int events_add(ape_event_descriptor *evd, int bitadd, ape_global *ape);
//@TODO: int events_mod(ape_event_descriptor *evd, int bitadd, ape_global *ape);
//@TODO: int events_del(int fd, ape_global *ape);
//@TODO: ape_event_descriptor *events_get_current_evd(struct _fdevent *ev, int i);
//@TODO: int events_poll(struct _fdevent *ev, int timeout_ms);
//@TODO: void events_shrink(struct _fdevent *ev);
//@TODO: void events_setsize(struct _fdevent *ev, int size);
//@TODO: int event_kqueue_init(struct _fdevent *ev);
//@TODO: int event_epoll_init(struct _fdevent *ev);
//@TODO: int event_select_init(struct _fdevent *ev);
//@TODO: int events_revent(struct _fdevent *ev, int i);

TEST(Events, Init)
{
	ape_global * g_ape;


	g_ape = APE_init();
	EXPECT_TRUE(g_ape->events.basemem > 0);
	EXPECT_EQ(g_ape->events.nfd, 0);
	EXPECT_TRUE(g_ape->events.handler != 0);

	APE_destroy(g_ape);
}

