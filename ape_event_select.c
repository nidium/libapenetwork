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

typedef enum
{
  evsb_none             = 0,
  evsb_added            = 1,    /**< descriptor has been added to list */
  evsb_ready            = 2,    /**< descriptor is ready to read or write */
  evsb_writeWatch       = 4     /**< descriptor should be watched for writes */
} evsb_bit_t;   /**< event status bits; must fit in a nibble */

static int event_select_add(struct _fdevent *ev, int fd, int bitadd,
        void *attach)
{
	printf("Adding %d to list %d\n", fd, FD_SETSIZE);
	if (fd < 0 || fd > FD_SETSIZE) {
		printf("cant add event %d\n", fd);
		return -1;
	}
  
	if (bitadd & EVENT_READ) 
		ev->fds[fd].read |= evsb_added;

	if (bitadd & EVENT_WRITE) 
		ev->fds[fd].write |= evsb_added | evsb_writeWatch;

	ev->fds[fd].fd = fd;
	ev->fds[fd].ptr = attach;
	printf("[++++] added %d\n", fd);


	return 1;
}

static int event_select_del(struct _fdevent *ev, int fd)
{

	ev->fds[fd].read = 0;
	ev->fds[fd].write = 0;

    return 1;
}

static int event_select_poll(struct _fdevent *ev, int timeout_ms)
{
  struct timeval        tv;
  int                   fd, i, maxfd, numfds;
  fd_set                rfds, wfds;

  if (timeout_ms < MIN_TIMEOUT_MS)
    timeout_ms = MIN_TIMEOUT_MS;

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;

  FD_ZERO(&rfds);
  FD_ZERO(&wfds);

  for (fd=0, maxfd=0; fd < FD_SETSIZE; fd++)
  {
    if (ev->fds[fd].read) {
        FD_SET((SOCKET)fd, &rfds);
    }
    if (ev->fds[fd].write & evsb_writeWatch) {
        FD_SET((SOCKET)fd, &wfds);
    }
    if (ev->fds[fd].read || ev->fds[fd].write & evsb_writeWatch)
    {
      if (fd > maxfd)
        maxfd = fd;
    }

  }

  errno = 0;
  numfds = select(maxfd + 1, &rfds, &wfds, NULL, &tv);
  switch(numfds)
  {
    case -1:
        Sleep(1000);
        fprintf(stderr, "Error calling select: %s, %d, %d, %d\n", strerror(SOCKERRNO), maxfd, numfds, SOCKERRNO); 
    case 0:
        return numfds;
  }
 
  /* Mark pending data */
  for (fd=0; fd <= maxfd; fd++)
  {
    if (FD_ISSET(fd, &rfds)) {
      ev->fds[fd].read |= evsb_ready;
	  
	}
    else
      ev->fds[fd].read &= ~evsb_ready;

    if (FD_ISSET(fd, &wfds)) {
      ev->fds[fd].write |= evsb_ready;
     
    }
    else
    {
      ev->fds[fd].write &= ~evsb_ready;
    }
  }
  
  /* Create the events array for event_select_revent et al */
  for (fd=0, i=0; fd <= maxfd; fd++)
  {
    if (FD_ISSET(fd, &rfds) || FD_ISSET(fd, &wfds))
    {
      ev->events[i++] = &ev->fds[fd];
    }
  }
  
  return i;
}

static void *event_select_get_fd(struct _fdevent *ev, int i)
{
	return ev->events[i]->ptr;
}

static void event_select_growup(struct _fdevent *ev)
{

}

static int event_select_revent(struct _fdevent *ev, int i)
{
	int bitret = 0;
	int fd = ev->events[i]->fd;

	if (ev->fds[fd].read & evsb_ready)
	bitret |= EVENT_READ;

	if (ev->fds[fd].write & evsb_ready)
	bitret |= EVENT_WRITE;

	ev->fds[fd].read &= evsb_added;       /* clear ready */
	ev->fds[fd].write &= evsb_added | evsb_writeWatch;    /* clear ready */

	return bitret;
}


int event_select_reload(struct _fdevent *ev)
{


    return 1;
}

int event_select_init(struct _fdevent *ev)
{
    ev->basemem = 2048;
	ev->events = malloc(sizeof(*ev->events) * (ev->basemem));
	memset(ev->fds, 0, sizeof(ev->fds));

	ev->add               = event_select_add;
	ev->poll              = event_select_poll;
	ev->get_current_fd    = event_select_get_fd;
	ev->revent            = event_select_revent;
	ev->reload            = event_select_reload;
    ev->setsize           = NULL;

    return 1;
}

#else
int event_select_init(struct _fdevent *ev)
{
    return 0;
}
#endif

// vim: ts=4 sts=4 sw=4 et

