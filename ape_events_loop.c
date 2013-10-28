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
#include "ape_timers.h"
#include "ape_timers_next.h"

#ifndef __WIN32
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stddef.h>

extern int ape_running;

void events_loop(ape_global *ape)
{
    int nfd, fd, bitev;
    
    void *attach;
    int nexttimeout = 1;
    //uint64_t start_monotonic = mach_absolute_time(), end_monotonic;

    while(ape->is_running && ape_running) {
        int i;
        printf("Next timeout in %d\n", nexttimeout);
        if ((nfd = events_poll(&ape->events, nexttimeout)) == -1) {
            continue;
        }
        for (i = 0; i < nfd; i++) {
			if ((attach  = events_get_current_fd(&ape->events, i)) == NULL) {
				continue;
			}
			
            bitev   = events_revent(&ape->events, i);
			//if (attach == NULL) printf("Will failed\n");
            fd  = ((ape_fds *)attach)->fd; /* assuming that ape_fds is the first member */
			//printf("Getting : %d on %d %d\n", i, fd, bitev);
			//if ((ape_fds *)attach)->state == ()
            switch(((ape_fds *)attach)->type) {

            case APE_SOCKET:
                if (APE_SOCKET(attach)->states.type == APE_SOCKET_TP_SERVER) {
                    if (bitev & EVENT_READ) {
                        if (APE_SOCKET(attach)->states.proto == APE_SOCKET_PT_TCP ||
							APE_SOCKET(attach)->states.proto == APE_SOCKET_PT_SSL) {
								
                            ape_socket_accept(APE_SOCKET(attach));
                        } else {
                            printf("read on UDP\n");
                        }
                    }
                } else if (APE_SOCKET(attach)->states.type == APE_SOCKET_TP_CLIENT) {

                    
                    /* unset this before READ event because read can invoke writes */
                    if (bitev & EVENT_WRITE) {
                        APE_SOCKET(attach)->states.flags &= ~APE_SOCKET_WOULD_BLOCK;
                    }

                    if (bitev & EVENT_READ &&
                        ape_socket_read(APE_SOCKET(attach)) == -1) {
                        
                        /* ape_socket is planned to be release after the for block */
                        continue;
                    }

                    if (bitev & EVENT_WRITE) {
                        if (APE_SOCKET(attach)->states.state == APE_SOCKET_ST_ONLINE &&
                                !(APE_SOCKET(attach)->states.flags & APE_SOCKET_WOULD_BLOCK)) {

                            ape_socket_do_jobs(APE_SOCKET(attach));
                            
                        } else if (APE_SOCKET(attach)->states.state == APE_SOCKET_ST_PROGRESS) {
                            int serror = 0, ret;
                            socklen_t serror_len = sizeof(serror);
							
                            if ((ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &serror, &serror_len)) == 0 &&
                                serror == 0) {
								
                                APE_SOCKET(attach)->states.state = APE_SOCKET_ST_ONLINE;

                                ape_socket_connected(APE_SOCKET(attach));
                                
                            } else {
                                APE_socket_destroy(APE_SOCKET(attach));
                            }
                        }
                    }
                } else if (APE_SOCKET(attach)->states.type == APE_SOCKET_TP_UNKNOWN) {

                }

                break;
            case APE_FILE:
                break;
            case APE_DELEGATE:
                ((struct _ape_fd_delegate *)attach)->on_io(fd, bitev, ape); /* punning */
                break;
            }
        }

        nexttimeout = process_timers(&ape->timersng);
    }
}

// vim: ts=4 sts=4 sw=4 et

