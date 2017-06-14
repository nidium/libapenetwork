/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_common.h"
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

static int event_epoll_add(struct _fdevent *ev, ape_event_descriptor *evd,
                           int bitadd)
{
    struct epoll_event kev;

    kev.data.u64 = 0;
    kev.events   = ((bitadd & EVENT_LEVEL ? 0 : EPOLLET)) | EPOLLPRI;

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

static int event_epoll_mod(struct _fdevent *ev, ape_event_descriptor *evd,
                           int bitadd)
{
    struct epoll_event kev;

    kev.data.u64 = 0;
    kev.events   = ((bitadd & EVENT_LEVEL ? 0 : EPOLLET)) | EPOLLPRI;

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

    if ((nfds = epoll_wait(ev->epoll_fd, ev->events, ev->basemem, timeout_ms))
        == -1) {
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
    ev->events = realloc(ev->events, sizeof(struct epoll_event) * (size));
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

#ifdef DEBUG
    APE_DEBUG("libapenetwork", "[Event] Event loop started using epoll()\n");
#endif
    return 1;
}

#else
int event_epoll_init(struct _fdevent *ev)
{
    return 0;
}
#endif

