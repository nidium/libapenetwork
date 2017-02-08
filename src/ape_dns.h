/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_DNS_H_
#define __APE_DNS_H_

#define _HAS_ARES_SUPPORT

#include "ape_common.h"

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
    int invalidate : 4;
    int done : 4;
} ape_dns_state;

void ape_dns_invalidate(ape_dns_state *state);
int ape_dns_init(ape_global *ape);

ape_dns_state *ape_gethostbyname(const char *host,
                                 ape_gethostbyname_callback callback, void *arg,
                                 ape_global *ape);

#ifdef __cplusplus
}
#endif

#endif

