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

#include "common.h"
#include "ape_events.h"
#include "ape_socket.h"
#include "ape_timers_next.h"
#include "ape_events_loop.h"

#ifndef __WIN32
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stddef.h>

extern int ape_running;

void events_loop(ape_global *ape)
{
    int nfd, fd, bitev;

    ape_event_descriptor *evd;
    int nexttimeout = 1;
    //uint64_t start_monotonic = mach_absolute_time(), end_monotonic;

    while (ape->is_running && ape_running) {
        int i;

        events_shrink(&ape->events);
        if ((nfd = events_poll(&ape->events, nexttimeout)) == -1) {
            continue;
        }

        for (i = 0; i < nfd; i++) {
            if ((evd  = events_get_current_evd(&ape->events, i)) == NULL) {
                continue;
            }

            bitev   = events_revent(&ape->events, i);

             /* assuming that ape_event_descriptor is the first member */
            fd  = evd->fd;

            switch (evd->type) {
            case APE_EVENT_SOCKET:
                if (APE_EVENT_SOCKET_PTR(evd)->states.type == APE_SOCKET_TP_SERVER) {
                    if (bitev & EVENT_READ) {
                        if (APE_EVENT_SOCKET_PTR(evd)->states.proto == APE_SOCKET_PT_TCP ||
                            APE_EVENT_SOCKET_PTR(evd)->states.proto == APE_SOCKET_PT_SSL) {
                            ape_socket_accept(APE_EVENT_SOCKET_PTR(evd));
                        } else {
                            ape_socket_read_udp(APE_EVENT_SOCKET_PTR(evd));
                        }
                    }
                } else if (APE_EVENT_SOCKET_PTR(evd)->states.type == APE_SOCKET_TP_CLIENT) {
                    /* unset this before READ event because read can invoke writes */
                    if (bitev & EVENT_WRITE) {
                        APE_EVENT_SOCKET_PTR(evd)->states.flags &= ~APE_SOCKET_WOULD_BLOCK;
                    }

                    if (APE_EVENT_SOCKET_PTR(evd)->states.proto != APE_SOCKET_PT_UDP &&
                        (bitev & EVENT_READ) &&
                        ape_socket_read(APE_EVENT_SOCKET_PTR(evd)) == -1) {

                        /* ape_socket is planned to be released after the for block */
                        continue;
                    } else if (APE_EVENT_SOCKET_PTR(evd)->states.proto ==
                        APE_SOCKET_PT_UDP && (bitev & EVENT_READ)) {

                        ape_socket_read_udp(APE_EVENT_SOCKET_PTR(evd));
                    }

                    if (bitev & EVENT_WRITE) {
                        if (APE_EVENT_SOCKET_PTR(evd)->states.state == APE_SOCKET_ST_ONLINE &&
                                !(APE_EVENT_SOCKET_PTR(evd)->states.flags & APE_SOCKET_WOULD_BLOCK)) {

                            /*
                                Execute the jobs list.
                                If the job list is done (returns 1), call drain callback.
                            */
                            if (ape_socket_do_jobs(APE_EVENT_SOCKET_PTR(evd)) == 1 &&
                                APE_EVENT_SOCKET_PTR(evd)->callbacks.on_drain != NULL) {
                                APE_EVENT_SOCKET_PTR(evd)->callbacks.on_drain(
                                    APE_EVENT_SOCKET_PTR(evd), ape,
                                    APE_EVENT_SOCKET_PTR(evd)->callbacks.arg);
                            }

                        } else if (APE_EVENT_SOCKET_PTR(evd)->states.state == APE_SOCKET_ST_PROGRESS) {
                            int ret;
                            char serror = '\0';
                            socklen_t serror_len = sizeof(serror);

                            if ((ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &serror, &serror_len)) == 0 &&
                                serror == 0) {

                                APE_EVENT_SOCKET_PTR(evd)->states.state = APE_SOCKET_ST_ONLINE;

                                ape_socket_connected(APE_EVENT_SOCKET_PTR(evd));

                            } else {
                                ape_socket_destroy(APE_EVENT_SOCKET_PTR(evd));
                            }
                        }
                    }
                } else if (APE_EVENT_SOCKET_PTR(evd)->states.type == APE_SOCKET_TP_UNKNOWN) {

                }

                break;
            case APE_EVENT_DELEGATE:
                ((struct _ape_fd_delegate *)evd)->on_io(fd, bitev,
                    ((struct _ape_fd_delegate *)evd)->data, ape); /* punning */
                break;
            default:
                break;
            }
        }
        nexttimeout = process_timers(&ape->timersng);
    }
}

