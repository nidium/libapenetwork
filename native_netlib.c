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

#include "native_netlib.h"
#include "common.h"
#include "ape_dns.h"
#include "ape_ssl.h"

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>


ape_global *native_netlib_init()
{
    ape_global *ape;
    struct _fdevent *fdev;

    if ((ape = malloc(sizeof(*ape))) == NULL) return NULL;

#ifndef __WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    fdev = &ape->events;
    fdev->handler = EVENT_UNKNOWN;
    #ifdef USE_EPOLL_HANDLER
    fdev->handler = EVENT_EPOLL;
    #endif
    #ifdef USE_KQUEUE_HANDLER
    fdev->handler = EVENT_KQUEUE;
    #endif
    #ifdef USE_SELECT_HANDLER
	fdev->handler = EVENT_SELECT;
    #endif

    ape->basemem    = APE_BASEMEM;
    ape->is_running = 1;
    ape->timers.ntimers = 0;
    ape->timers.timers  = NULL;
    ape->timersng.run_in_low_resolution = 0;

    ape->timersng.head = NULL;
    ape->timersng.last_identifier = 0;
    ape->ctx = NULL;
	
    ape_dns_init(ape);
#ifdef _HAVE_SSL_SUPPORT
    ape_ssl_init();
    if ((ape->ssl_global_ctx = ape_ssl_init_global_client_ctx()) == NULL) {
        printf("SSL: failed to init global CTX\n");
    }
#endif
    events_init(ape);
	
    return ape;
}
