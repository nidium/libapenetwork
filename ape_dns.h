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

#ifndef __APE_DNS_H_
#define __APE_DNS_H_

#define _HAS_ARES_SUPPORT

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ape_gethostbyname_callback)(const char *ip, void *arg,
        int status);


struct _ares_sockets {
    _APE_FD_DELEGATE_TPL
};

typedef struct _ape_dns_cb_argv {
    ape_global *ape;
    ape_gethostbyname_callback callback;
    const char *origin;
    void *arg;
    int invalidate:4;
    int done:4;
} ape_dns_state;

void ape_dns_invalidate(ape_dns_state *state);
int ape_dns_init(ape_global *ape);

ape_dns_state *ape_gethostbyname(const char *host, ape_gethostbyname_callback callback,
        void *arg, ape_global *ape);

#ifdef __cplusplus
}
#endif

#endif


