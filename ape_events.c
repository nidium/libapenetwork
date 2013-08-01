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

#include <stdlib.h>


int events_add(int fd, void *attach, int bitadd, ape_global *ape)
{
    struct _fdevent *ev = &ape->events;

    if (ev->add(ev, fd, bitadd, attach) == -1) {
        return -1;
    }

    return 1;
}

int events_del(int fd, ape_global *ape)
{
    struct _fdevent *ev = &ape->events;

    if (ev->del(ev, fd) == -1) {
        return -1;
    }

    return 1;
}

int events_poll(struct _fdevent *ev, int timeout_ms)
{
    int nfds;

    if ((nfds = ev->poll(ev, timeout_ms)) == -1) {
        return -1;
    }

    return nfds;
}


void *events_get_current_fd(struct _fdevent *ev, int i)
{
    return ev->get_current_fd(ev, i);
}

/*
void events_growup(struct _fdevent *ev)
{
    ev->growup(ev);
}
*/
int events_revent(struct _fdevent *ev, int i)
{
    return ev->revent(ev, i);
}

/*
int events_reload(struct _fdevent *ev)
{
    return ev->reload(ev);
}
*/

int events_init(ape_global *ape)
{
    ape->events.basemem = &ape->basemem;

    switch(ape->events.handler) {
        case EVENT_EPOLL:
            return event_epoll_init(&ape->events);
            break;
        case EVENT_KQUEUE:
            return event_kqueue_init(&ape->events);
            break;
		case EVENT_SELECT:
            return event_select_init(&ape->events);
			break;
        default:
            break;
    }

    return -1;
}

// vim: ts=4 sts=4 sw=4 et

