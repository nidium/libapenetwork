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
#ifndef __WIN32
#include <sys/time.h>
#include <unistd.h>
#else
#include "port\windows.h"
#endif
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include "ape_socket.h"

#ifdef USE_SELECT_HANDLER

#ifndef MIN_TIMEOUT_MS
# ifdef DEBUG_EVENT_SELECT
#  define MIN_TIMEOUT_MS        1
# else
#  define MIN_TIMEOUT_MS        150
# endif
#endif

enum {
    kWatchForRead_Event  = 1 << 0,
    kWatchForWrite_Event = 1 << 1,
    kWatchForError_Event = 1 << 2,

    kReadyForRead_Event   = 1 << 3,
    kReadyForWrite_Event  = 1 << 4,
    kIsReady_Event        = (kReadyForRead_Event | kReadyForWrite_Event)
};

typedef struct select_fdinfo_t
{
    void *ptr;
    int fd;
    char watchfor:8;
} select_fdinfo_t;

static int event_select_add(struct _fdevent *ev, int fd, int bitadd,
        void *attach)
{
    if (fd < 0 || fd > FD_SETSIZE) {
        printf("cant add event %d\n", fd);
        return -1;
    }

    select_fdinfo_t *fdinfo = malloc(sizeof(select_fdinfo_t));
    fdinfo->fd = fd;
    fdinfo->ptr = attach;
    fdinfo->watchfor = 0;
  
    if (bitadd & EVENT_READ) {
        fdinfo->watchfor |= kWatchForRead_Event;
    }

    if (bitadd & EVENT_WRITE) {
        fdinfo->watchfor |= kWatchForWrite_Event;
    }

    hashtbl_append64(ev->fdhash, fd, fdinfo);

    printf("[++++] added %d\n", fd);

    return 1;
}

static int event_select_del(struct _fdevent *ev, int fd)
{
    hashtbl_erase64(ev->fdhash, fd);

    return 1;
}

static int event_select_poll(struct _fdevent *ev, int timeout_ms)
{
    ape_htable_item_t *item;
    struct timeval        tv;
    int                   fd, i, numfds;
    fd_set                rfds, wfds;
    int maxfd = 0, tmpfd = 0;

    if (timeout_ms < MIN_TIMEOUT_MS) {
        timeout_ms = MIN_TIMEOUT_MS;
    }

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    for (item = ev->fdhash->first; item != NULL; item = item->lnext, tmpfd = 0) {
        select_fdinfo_t *fdinfo = (select_fdinfo_t *)item->content.addrs;

        fdinfo->watchfor &= ~kIsReady_Event;

        if (fdinfo->watchfor & kWatchForRead_Event) {
            FD_SET(fdinfo->fd, &rfds);
            tmpfd = fdinfo->fd;
        }
        if (fdinfo->watchfor & kWatchForWrite_Event) {
            FD_SET(fdinfo->fd, &wfds);
            tmpfd = fdinfo->fd;
        }

        if (tmpfd > maxfd) {
            maxfd = tmpfd;
        }
    }

    if (!maxfd) {
        numfds = 0;
        usleep((timeout_ms % 1000) * 1000);
    } else {
        numfds = select(maxfd + 1, &rfds, &wfds, NULL, &tv);
    }
    switch(numfds)
    {
        case -1:
            fprintf(stderr, "Error calling select: %s, %d, %d, %d\n", strerror(SOCKERRNO), maxfd, numfds, SOCKERRNO);
            exit(1);
        case 0:
            return numfds;
    }

    /* Mark pending data */
    for (fd = 0; fd <= maxfd; fd++) {
        select_fdinfo_t *fdinfo = NULL;

        if (FD_ISSET(fd, &rfds)) {
            fdinfo = hashtbl_seek64(ev->fdhash, fd);
            if (!fdinfo) {
                printf("assert failed, select() returned an unknow fd (read)\n");
                continue;
            }
            
            fdinfo->watchfor |= kReadyForRead_Event;
        }

        if (FD_ISSET(fd, &wfds)) {
            if (!fdinfo) {
                fdinfo = hashtbl_seek64(ev->fdhash, fd);
                if (!fdinfo) {
                    printf("assert failed, select() returned an unknow fd (write)\n");
                    continue;
                }               
            }
            fdinfo->watchfor |= kReadyForWrite_Event;
        }
    }

    /* Create the events array for event_select_revent et al */
    for (fd = 0, i = 0; fd <= maxfd; fd++)
    {
        if (FD_ISSET(fd, &rfds) || FD_ISSET(fd, &wfds)) {
            ev->events[i++] = (select_fdinfo_t *)hashtbl_seek64(ev->fdhash, fd);
        }
    }

    return i;
}

static void *event_select_get_fd(struct _fdevent *ev, int i)
{
    return ev->events[i]->ptr;
}

static int event_select_revent(struct _fdevent *ev, int i)
{
    select_fdinfo_t *fdinfo = ev->events[i];

    int bitret = 0;

    if (fdinfo->watchfor & kReadyForRead_Event) {
        bitret |= EVENT_READ;
    }

    if (fdinfo->watchfor & kReadyForWrite_Event) {
        bitret |= EVENT_WRITE;
    }

    fdinfo->watchfor &= ~kIsReady_Event;

    return bitret;
}


static int event_select_reload(struct _fdevent *ev)
{   
    /* Do nothing (?) */

    return 1;
}

static void event_select_setsize(struct _fdevent *ev, int size)
{
    ev->basemem = FD_SETSIZE;
    if (size > FD_SETSIZE) {
        printf("[Socket error] event_select_setsize requested a size > FD_SETSIZE (%d > %d)\n",
            size, FD_SETSIZE);
    }
    /* Do nothing */
}


static void event_select_clean_fd(ape_htable_item_t *item)
{
    /* release select_fdinfo_t */
    free(item->content.addrs);
}


int event_select_init(struct _fdevent *ev)
{
    ev->basemem = FD_SETSIZE;

    ev->events = malloc(sizeof(*ev->events) * (ev->basemem));

    ev->fdhash            = hashtbl_init(APE_HASH_INT);
    hashtbl_set_cleaner(ev->fdhash, event_select_clean_fd);

    ev->add               = event_select_add;
    ev->del               = event_select_del;
    ev->poll              = event_select_poll;
    ev->get_current_fd    = event_select_get_fd;
    ev->revent            = event_select_revent;
    ev->reload            = event_select_reload;
    ev->setsize           = event_select_setsize;

    printf("select() started with %i slots\n", ev->basemem);

    return 1;
}

#else
int event_select_init(struct _fdevent *ev)
{
    return 0;
}
#endif

// vim: ts=4 sts=4 sw=4 et

