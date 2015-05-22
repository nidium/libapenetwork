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

#include <ares.h>

#ifdef _WIN32
#include <WinSock2.h>
#endif

ape_global *native_netlib_init()
{
    ape_global *ape;
    struct _fdevent *fdev;

#ifndef __WIN32
    signal(SIGPIPE, SIG_IGN);
#else
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("WSA failed\n");
        return NULL;
    }
#endif

    if ((ape = malloc(sizeof(*ape))) == NULL) return NULL;
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

    ape->is_running = 1;

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

void native_netlib_destroy(ape_global * ape)
{
    //  destroying dns
    struct _ares_sockets *as;
    size_t i;

    ares_cancel(ape->dns.channel);
    as = ape->dns.sockets.list;

    for(i = 0 ; i < ape->dns.sockets.size; i++) {
        events_del(as->s.fd, ape);
        as++;
    }

    free(ape->dns.sockets.list);
    ape->dns.sockets.size = 0;
    ape->dns.sockets.used = 0;
    ares_destroy(ape->dns.channel);
    ares_library_cleanup();

    //  destroying events
    events_destroy(&ape->events);
    // destroying timers
    del_timers_all(&ape->timersng);

    //  destroying rest
    free(ape);
}
