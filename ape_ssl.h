/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_SSL_H
#define __APE_SSL_H

#ifdef _HAVE_SSL_SUPPORT

#include <openssl/ssl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ape_ssl {
    SSL_CTX *ctx;
    SSL     *con;
} ape_ssl_t;

void ape_ssl_library_init();
void ape_ssl_library_destroy();
ape_ssl_t *ape_ssl_init_ctx(const char *cert, const char *key);
ape_ssl_t *ape_ssl_init_con(ape_ssl_t *parent, int fd, int accept);
ape_ssl_t *ape_ssl_init_global_client_ctx();
int ape_ssl_read(ape_ssl_t *ssl, void *buf, int num);
int ape_ssl_write(ape_ssl_t *ssl, void *buf, int num);
void ape_ssl_shutdown(ape_ssl_t *ssl);
void ape_ssl_destroy(ape_ssl_t *ssl);

#ifdef __cplusplus
}
#endif

#endif

#endif

