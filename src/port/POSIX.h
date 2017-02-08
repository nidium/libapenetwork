/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_PORT_POSIX_H
#define __APE_PORT_POSIX_H

#include <errno.h>

#define sclose(x) close((x))
#define swrite(fd, data, bytes) write(fd, data, bytes)
#define sread(fd, buf, len) read(fd, buf, len)

#define SOCKERRNO (errno)
#define SET_SOCKERRNO(x) (errno = (x))

typedef int sockopt_t;

#endif

