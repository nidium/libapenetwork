/*
    APE Network Library
    Copyright (C) 2010-2014 Anthony Catel <paraboul@gmail.com>

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
#include <stdio.h>

#define APE_DEFAULT_EVENTS_SIZE 32


int events_add(int fd, void *attach, int bitadd, ape_global *ape)
{
    struct _fdevent *ev = &ape->events;

    ev->nfd++;

    if (ev->nfd > ev->basemem) {
        events_setsize(ev, ev->basemem << 1);
    }

    if (ev->add(ev, fd, bitadd, attach) == -1) {
        return -1;
    }

    return 1;
}

int events_del(int fd, ape_global *ape)
{

    ape->events.nfd--;

    /*if (ev->del(ev, fd) == -1) {
        return -1;
    }*/

    return 1;
}

/*
    http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
*/
static unsigned int _nextpoweroftwo(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void events_shrink(struct _fdevent *ev)
{
#ifdef _WIN32
    return;
#endif
    int curmem = ev->basemem;

    if (curmem == APE_DEFAULT_EVENTS_SIZE) {
        return;
    }

    int prevmem = curmem >> 1;

    if (ev->nfd >= prevmem) {
        return;
    }

    events_setsize(ev, ape_max(APE_DEFAULT_EVENTS_SIZE, _nextpoweroftwo(ev->nfd)));
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


void events_setsize(struct _fdevent *ev, int size)
{
    ev->basemem = size;
    ev->setsize(ev, size);
}

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
    ape->events.basemem = APE_DEFAULT_EVENTS_SIZE;
    ape->events.nfd = 0;

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

