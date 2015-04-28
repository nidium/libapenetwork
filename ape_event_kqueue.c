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
#ifndef __WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <time.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef USE_KQUEUE_HANDLER

static int event_kqueue_add(struct _fdevent *ev,
	ape_event_descriptor *evd, int bitadd)
{
	struct kevent kev;
	struct timespec ts;
	int baseflag = EV_ADD | (bitadd & EVENT_LEVEL ? 0 : EV_CLEAR);
		
	memset(&kev, 0, sizeof(kev));

	ts.tv_sec = 0;
	ts.tv_nsec = 0;	
	
	if (bitadd & EVENT_READ) {
	
		EV_SET(&kev, evd->fd, EVFILT_READ, baseflag, 0, 0, evd);
		if (kevent(ev->kq_fd, &kev, 1, NULL, 0, &ts) == -1) {
			return -1;
		}
	
	}

	if (bitadd & EVENT_WRITE) {
		ts.tv_sec = 0;
		ts.tv_nsec = 0;

		memset(&kev, 0, sizeof(kev));
	
		EV_SET(&kev, evd->fd, EVFILT_WRITE, baseflag, 0, 0, evd);
		if (kevent(ev->kq_fd, &kev, 1, NULL, 0, &ts) == -1) {
			return -1;
		}
	
	}

	return 1;
}

static int event_kqueue_mod(struct _fdevent *ev,
    ape_event_descriptor *evd, int bitadd)
{
	/*
		Re-adding an existing event will modify the parameters of the original
		event, and not result in a duplicate entry.
	*/
	return event_kqueue_add(ev, evd, bitadd);
}

static int event_kqueue_poll(struct _fdevent *ev, int timeout_ms)
{
	int nfds;
	struct timespec ts;
	
	ts.tv_sec = timeout_ms / 1000;
	ts.tv_nsec = (timeout_ms % 1000) * 1000000;

	if ((nfds = kevent(ev->kq_fd, NULL, 0, ev->events, ev->basemem * 2, &ts)) == -1) {
		return -1;
	}

	return nfds;
}

static ape_event_descriptor *event_kqueue_get_evd(struct _fdevent *ev, int i)
{
	if (((ape_socket *)ev->events[i].udata)->states.state == APE_SOCKET_ST_OFFLINE) {
		return NULL;
	}
	return (ape_event_descriptor *)ev->events[i].udata;
}

static void event_kqueue_setsize(struct _fdevent *ev, int size)
{
	struct kevent *tmp;

	tmp = realloc(ev->events, sizeof(struct kevent) * (size * 2));
	if( tmp != NULL) {
		ev->events = tmp;
	}
}

static int event_kqueue_revent(struct _fdevent *ev, int i)
{
	int bitret = 0;
	
	if (ev->events[i].filter == EVFILT_READ) {
		bitret = EVENT_READ;
	} else if (ev->events[i].filter == EVFILT_WRITE) {
		bitret = EVENT_WRITE;
	}
	
	return bitret;
}


static int event_kqueue_reload(struct _fdevent *ev)
{
	int nfd;
	if ((nfd = dup(ev->kq_fd)) != -1) {
		close(nfd);
		close(ev->kq_fd);
	}

	if ((ev->kq_fd = kqueue()) == -1) {
		return 0;
	}
	return 1;
}

int event_kqueue_init(struct _fdevent *ev)
{
	if ((ev->kq_fd = kqueue()) == -1) {
		printf("Kqueue failed\n");
		return 0;
	}

	ev->events = malloc(sizeof(struct kevent) * (ev->basemem * 2));
	memset(ev->events, 0, sizeof(struct kevent) * (ev->basemem * 2));
	
	ev->add 			= event_kqueue_add;
	ev->poll 			= event_kqueue_poll;
	ev->get_current_evd = event_kqueue_get_evd;
	ev->setsize 		= event_kqueue_setsize;
	ev->revent 			= event_kqueue_revent;
	ev->reload 			= event_kqueue_reload;
	ev->del				= NULL;
	ev->mod				= event_kqueue_mod;

	printf("Event loop started using kqueue()\n");
	
	return 1;
}

#else
int event_kqueue_init(struct _fdevent *ev)
{
	return 0;
}
#endif


