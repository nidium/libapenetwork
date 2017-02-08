/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_common.h"
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

void APE_loop_stop()
{
    ape_running = 0;
}

void APE_loop_run(ape_global *ape)
{
    int nfd, fd, bitev;

    ape_event_descriptor *evd;
    int nexttimeout = 1;
// uint64_t start_monotonic = mach_absolute_time(), end_monotonic;
event_loop:
    while (ape->is_running && ape_running) {
        ape_socket *cursocket;
        int i;

        events_shrink(&ape->events);
        if ((nfd = events_poll(&ape->events, nexttimeout)) == -1) {
            continue;
        }

        for (i = 0; i < nfd; i++) {
            if ((evd = events_get_current_evd(&ape->events, i)) == NULL) {
                continue;
            }

            cursocket = APE_EVENT_SOCKET_PTR(evd);

            bitev = events_revent(&ape->events, i);

            /* assuming that ape_event_descriptor is the first member */
            fd = evd->fd;

            switch (evd->type) {
                case APE_EVENT_SOCKET:
                    if (cursocket->states.type == APE_SOCKET_TP_SERVER) {
                        if (bitev & EVENT_READ) {
                            if (cursocket->states.proto == APE_SOCKET_PT_TCP
                                || cursocket->states.proto
                                       == APE_SOCKET_PT_SSL) {
                                ape_socket_accept(cursocket);
                            } else {
                                ape_socket_read_udp(cursocket);
                            }
                        }
                    } else if (cursocket->states.type == APE_SOCKET_TP_CLIENT) {
                        /* unset this before READ event because read can invoke
                         * writes */
                        if (bitev & EVENT_WRITE) {
                            cursocket->states.flags &= ~APE_SOCKET_WOULD_BLOCK;
                        }

                        if (cursocket->states.proto != APE_SOCKET_PT_UDP
                            && (bitev & EVENT_READ)
                            && ape_socket_read(cursocket) == -1) {

                            /* ape_socket is planned to be released after the
                             * for block */
                            continue;
                        } else if (cursocket->states.proto == APE_SOCKET_PT_UDP
                                   && (bitev & EVENT_READ)) {

                            ape_socket_read_udp(cursocket);
                        }

                        if (bitev & EVENT_WRITE) {
                            if (cursocket->states.state == APE_SOCKET_ST_ONLINE
                                && !(cursocket->states.flags
                                     & APE_SOCKET_WOULD_BLOCK)) {

                                /*
                                    Execute the jobs list.
                                    If the job list is done (returns 1), call
                                   drain callback.
                                */
                                // APE_DEBUG("libapenetwork", "[Loop] Do job...%d\n",
                                // (cursocket->states.events_flags &
                                // EVENT_WRITE));
                                if (ape_socket_do_jobs(cursocket) == 1
                                    && cursocket->callbacks.on_drain != NULL) {
                                    cursocket->callbacks.on_drain(
                                        cursocket, ape,
                                        cursocket->callbacks.arg);
                                }

                            } else if (cursocket->states.state
                                       == APE_SOCKET_ST_PROGRESS) {
                                int ret;
                                char serror          = '\0';
                                socklen_t serror_len = sizeof(serror);

                                if ((ret = getsockopt(fd, SOL_SOCKET, SO_ERROR,
                                                      &serror, &serror_len))
                                        == 0
                                    && serror == 0) {

                                    cursocket->states.state
                                        = APE_SOCKET_ST_ONLINE;

                                    ape_socket_connected(cursocket);

                                } else {
                                    ape_socket_destroy(cursocket);
                                }
                            }
                        }
                    } /* else if (cursocket->states.type ==
                     APE_SOCKET_TP_UNKNOWN) {

                     }*/

                    break;
                case APE_EVENT_DELEGATE:
                    ((struct _ape_fd_delegate *)evd)
                        ->on_io(fd, bitev,
                                ((struct _ape_fd_delegate *)evd)->data,
                                ape); /* punning */
                    break;
                default:
                    break;
            }
        }
        nexttimeout = ape_timers_process(ape);
    }

    if (ape->is_running && ape->kill_handler && ape->kill_handler(0, ape)) {
        ape_running = 1;
        goto event_loop;
    }
#ifdef DEBUG
    APE_DEBUG("libapenetwork", "[Loop] exiting event loop...\n");
#endif
}

