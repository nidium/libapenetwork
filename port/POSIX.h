/*
Copyright 2016 Nidium Inc. All rights reserved.
Use of this source code is governed by a MIT license
that can be found in the LICENSE file.
*/

#ifndef __APE_PORT_POSIX_H
#define __APE_PORT_POSIX_H

#include <errno.h>

#define sclose(x)  close((x))


#define SOCKERRNO         (errno)
#define SET_SOCKERRNO(x)  (errno = (x))

#endif
