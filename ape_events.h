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

#ifndef __APE_EVENTS_H_
#define __APE_EVENTS_H_

#include "common.h"

#ifdef USE_KQUEUE_HANDLER
#include <sys/event.h>
#endif
#ifdef USE_EPOLL_HANDLER
#include <sys/epoll.h>
#endif
#ifdef USE_SELECT_HANDLER
  #ifndef _WIN32
    #include <sys/select.h>
  #endif
  #ifndef FD_SETSIZE
    #error "FD_SETSIZE NOT DEFINED"
  #endif
#endif

#include "ape_hash.h"

/* Generics flags */
#define EVENT_READ 0x01
#define EVENT_WRITE 0x02
#define EVENT_LEVEL 0x04
#define EVENT_CANCEL 0x08

#define _APE_FD_DELEGATE_TPL  \
    ape_event_descriptor s; \
    void (*on_io)(int fd, int ev, void *data, ape_global *ape); \
    void *data;

typedef enum {
    EVENT_UNKNOWN,
    EVENT_EPOLL,    /* Linux */
    EVENT_KQUEUE,   /* BSD */
    EVENT_DEVPOLL,  /* Solaris */
    EVENT_POLL,     /* POSIX */
    EVENT_SELECT    /* Generic (Windows) */
} fdevent_handler_t;

typedef enum {
    APE_EVENT_SOCKET,
    APE_EVENT_DELEGATE
} ape_event_t;

typedef struct {
    int fd;
    ape_event_t type;
} ape_event_descriptor;

struct _ape_fd_delegate {
    _APE_FD_DELEGATE_TPL
};

struct _fdevent {
    /* Interface */
    int (*add)      (struct _fdevent *ev, int fd, int bitadd, void *attach);
    int (*mod)      (struct _fdevent *ev, int fd, int bitadd);
    int (*del)      (struct _fdevent *ev, int fd);
    int (*poll)     (struct _fdevent *, int timeoutms);
    int (*revent)   (struct _fdevent *, int idx);
    int (*reload)   (struct _fdevent *ev);
    void (*setsize)  (struct _fdevent *, int size);
    void *(*get_current_fd) (struct _fdevent *, int idx);

    /* Specifics values */
#ifdef USE_KQUEUE_HANDLER
    struct kevent *events;
    int kq_fd;
#elif defined USE_EPOLL_HANDLER
    struct epoll_event *events;
    int epoll_fd;
#elif defined USE_SELECT_HANDLER
	struct select_fdinfo_t **events;       /* Pointers into fds */
    ape_htable_t *fdhash;
#endif
    int basemem;                /* Number of elements in events */
    int nfd;                    /* Number of managed file descriptor */
    fdevent_handler_t handler;  /* Type of handler (enum) */
};

#ifdef __cplusplus
extern "C" {
#endif

int events_init(ape_global *ape);
int events_add(int fd, void *attach, int bitadd, ape_global *ape);
int events_mod(int fd, int bitadd, ape_global *ape);
int events_del(int fd, ape_global *ape);
void *events_get_current_fd(struct _fdevent *ev, int i);
int events_poll(struct _fdevent *ev, int timeout_ms);
void events_shrink(struct _fdevent *ev);
void events_setsize(struct _fdevent *ev, int size);

int event_kqueue_init(struct _fdevent *ev);
int event_epoll_init(struct _fdevent *ev);
int event_select_init(struct _fdevent *ev);
int events_revent(struct _fdevent *ev, int i);

#ifdef __cplusplus
}
#endif


#endif



// vim: ts=4 sts=4 sw=4 et

