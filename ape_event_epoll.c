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
#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include "ape_socket.h"

#ifdef USE_EPOLL_HANDLER

static int event_epoll_add(struct _fdevent *ev,
    ape_event_descriptor *evd, int bitadd)
{
    struct epoll_event kev;

    kev.events = ((bitadd & EVENT_LEVEL ? 0 : EPOLLET)) | EPOLLPRI;

    if (bitadd & EVENT_READ) {
        kev.events |= EPOLLIN;
    }

    if (bitadd & EVENT_WRITE) {
        kev.events |= EPOLLOUT;
    }

    kev.data.ptr = evd;

    if (epoll_ctl(ev->epoll_fd, EPOLL_CTL_ADD, evd->fd, &kev) == -1) {
        return -1;
    }

    return 1;
}

static int event_epoll_mod(struct _fdevent *ev,
    ape_event_descriptor *evd, int bitadd)
{
    struct epoll_event kev;

    kev.events = ((bitadd & EVENT_LEVEL ? 0 : EPOLLET)) | EPOLLPRI;

    if (bitadd & EVENT_READ) {
        kev.events |= EPOLLIN;
    }

    if (bitadd & EVENT_WRITE) {
        kev.events |= EPOLLOUT;
    }

    kev.data.ptr = evd;

    if (epoll_ctl(ev->epoll_fd, EPOLL_CTL_MOD, evd->fd, &kev) == -1) {
        return -1;
    }
    return 1;
}

static int event_epoll_poll(struct _fdevent *ev, int timeout_ms)
{
    int nfds;
    if ((nfds = epoll_wait(ev->epoll_fd, ev->events, ev->basemem,
                    timeout_ms)) == -1) {
        return -1;
    }

    return nfds;
}

static ape_event_descriptor *event_epoll_get_evd(struct _fdevent *ev, int i)
{
    /* the value must start by ape_fds */
    return (ape_event_descriptor *)ev->events[i].data.ptr;
}

static void event_epoll_setsize(struct _fdevent *ev, int size)
{
    struct epoll_event *tmp;

    tmp = realloc(ev->events,
            sizeof(struct epoll_event) * (size));
    if (tmp != NULL) {
        ev->events = tmp;
    }
}

static int event_epoll_revent(struct _fdevent *ev, int i)
{
    int bitret = 0;

    if (ev->events[i].events & EPOLLIN) {
        bitret = EVENT_READ;
    }
    if (ev->events[i].events & EPOLLOUT) {
        bitret |= EVENT_WRITE;
    }

    return bitret;
}


static int event_epoll_reload(struct _fdevent *ev)
{
    int nfd;
    if ((nfd = dup(ev->epoll_fd)) != -1) {
        close(nfd);
        close(ev->epoll_fd);
    }
    if ((ev->epoll_fd = epoll_create(1)) == -1) {
        return 0;
    }

    return 1;
}

int event_epoll_init(struct _fdevent *ev)
{
    if ((ev->epoll_fd = epoll_create(1)) == -1) {
        return 0;
    }

    ev->events = malloc(sizeof(struct epoll_event) * (ev->basemem));

    ev->add             = event_epoll_add;
    ev->del             = NULL;
    ev->poll            = event_epoll_poll;
    ev->get_current_evd = event_epoll_get_evd;
    ev->setsize         = event_epoll_setsize;
    ev->revent          = event_epoll_revent;
    ev->reload          = event_epoll_reload;
    ev->mod             = event_epoll_mod;

    printf("Event loop started using epoll()\n");

    return 1;
}

#else
int event_epoll_init(struct _fdevent *ev)
{
    return 0;
}
#endif

