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

#include "ape_netlib.h"
#include "ape_common.h"
#include "ape_dns.h"
#include "ape_ssl.h"

#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ares.h>

#ifdef _WIN32
#include <WinSock2.h>
#endif

int ape_running = 1;

ape_global *APE_init()
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
    ape->timersng.head_async = NULL;
    ape->timersng.last_identifier = 0;

    ape->ctx = NULL;
    ape->kill_handler = NULL;

    ape_dns_init(ape);
#ifdef _HAVE_SSL_SUPPORT
    ape_ssl_library_init();
    if ((ape->ssl_global_ctx = ape_ssl_init_global_client_ctx()) == NULL) {
        printf("SSL: failed to init global CTX\n");
    }
#endif
    events_init(ape);

    ape->failed_write_count    = 0;
    ape->total_memory_buffered = 0;

    ape->urandom_fd = open("/dev/urandom", O_RDONLY);

    if (!ape->urandom_fd) {
        fprintf(stderr, "Cannot open /dev/urandom\n");
        NULL;
    }

    return ape;
}

void APE_destroy(ape_global * ape)
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
    APE_timers_destroy_all(ape);

#ifdef _HAVE_SSL_SUPPORT
    if (ape->ssl_global_ctx) {
        ape_ssl_shutdown(ape->ssl_global_ctx);
        ape_ssl_destroy(ape->ssl_global_ctx);
    }
    ape_ssl_library_destroy();
#endif
    close(ape->urandom_fd);
    //  destroying rest
    free(ape);
}

