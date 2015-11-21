﻿/*
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

#include <stdlib.h>

#define APE_DEFAULT_EVENTS_SIZE 32

int events_add(ape_event_descriptor *evd, int bitadd, ape_global *ape)
{
    struct _fdevent *ev = &ape->events;

    ev->nfd++;

    if (ev->nfd > ev->basemem) {
        events_setsize(ev, ev->basemem << 1);
    }

    if (ev->add(ev, evd, bitadd) == -1) {
        return -1;
    }

    return 1;
}

int events_mod(ape_event_descriptor *evd, int bitadd, ape_global *ape)
{
    struct _fdevent *ev = &ape->events;

    return ev->mod ? ev->mod(ev, evd, bitadd) : -1;
}

int events_del(int fd, ape_global *ape)
{
    struct _fdevent *ev = &ape->events;
    ape->events.nfd--;

    if (ev->del) {
        ev->del(ev, fd);
    }

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


ape_event_descriptor *events_get_current_evd(struct _fdevent *ev, int i)
{
    return ev->get_current_evd(ev, i);
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
        case EVENT_UNKNOWN:
        case EVENT_DEVPOLL:
        case EVENT_POLL:
        default:
            break;
    }

    return -1;
}

void events_destroy(struct _fdevent *ev) {
    /* free should be delegated to the corresponding subclass */
    free(ev->events); // @TODO:  del events and close fd's?

    ev->add     = NULL;
    ev->mod     = NULL;
    ev->del     = NULL;
    ev->poll    = NULL;
    ev->revent  = NULL;
    ev->reload  = NULL;
    ev->setsize = NULL;
    ev->get_current_evd = NULL;
}

