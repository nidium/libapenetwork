/*
Copyright 2016 Nidium Inc. All rights reserved.
Use of this source code is governed by a MIT license
that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#if _HAVE_SSL_SUPPORT

#include <ape_ssl.h>

TEST(SSL, Simple)
{
    ape_ssl_t *ssl;

    ape_ssl_library_init();

    ssl = NULL;
    ssl = ape_ssl_init_global_client_ctx();

    EXPECT_TRUE(ssl != NULL);
    EXPECT_TRUE(ssl->ctx != NULL);
    EXPECT_TRUE(ssl->con == NULL);

    ape_ssl_destroy(ssl);
    ape_ssl_library_destroy();
}

/*
@TODO: ape_ssl_init_ctx
@TODO: ape_ssl_shutdown(sslCtx);
@TODO: ape_ssl_read
@TODO: ape_ssl_write
*/

#endif

