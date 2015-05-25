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

void ape_ssl_init();
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
