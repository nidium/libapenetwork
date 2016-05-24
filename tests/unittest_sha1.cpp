/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_sha1.h>
#include <ape_base64.h>

#define ORG "The quick brown fox jumps over the lazy dog"
#define GRO "L9ThxnotKPzthJ7hu3bnORuT6xI="
#define GRO_MAC "GY6h6gTENcEka1hqBtXPEcP/zaY="

TEST(SHA1, Simple)
{
    sha1_context cx;
    char * org, pd[29];
    uchar digest[21];

    sha1_starts(&cx);
    org = strdup(ORG);
    sha1_update(&cx, (uchar*)org, strlen(org));
    sha1_finish(&cx, digest);
    base64_encode_b( digest, pd, 20);
    EXPECT_TRUE(strcmp(pd, GRO) == 0);

    free(org);
}

TEST(SHA1, Checksum)
{
    char * org, pd[29];
    uchar digest[21];

    org = strdup(ORG);
    sha1_csum( (uchar*)org, strlen(org), digest);
    base64_encode_b( digest, pd, 20);
    EXPECT_TRUE(strcmp(pd, GRO) == 0);

    free(org);
}
/*
TEST(SHA1, File)
{
    int success;
    uchar digest[21];
    char * filename;

    memset(&digest, '\0', 21);
    filename = strdup("./ape_sha1.h");
    success = sha1_file(filename, digest);
    EXPECT_EQ(success, 0);
    EXPECT_EQ(strlen((char*)digest), 20 );
    free(filename);
}
*/

TEST(SHA1, Hmac)
{
    char * org, *key, pd[29];
    uchar digest[21];

    org = strdup(ORG);
    key = strdup("secret");
    sha1_hmac( (uchar*) key, strlen(key), (uchar*)org, strlen(org), digest);
    base64_encode_b( digest, pd, 20);
    EXPECT_TRUE(strcmp(pd, GRO_MAC) == 0);

    free(key);
    free(org);
}

#undef ORG
#undef GRO
#undef GRO_MAC

