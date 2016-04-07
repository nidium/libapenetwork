/*
Copyright 2016 Nidium Inc. All rights reserved.
Use of this source code is governed by a MIT license
that can be found in the LICENSE file.
*/

#include <stdlib.h>
#include <string.h>

#include "unittest.h"

#include <ape_blowfish.h>

#define KEY_LEN 24
#define ORG "The best preparation for tomorrow is doing your best today."
static const uint8_t key[KEY_LEN] =  { 0x79, 0x34, 0x77, 0x4c, 0x67, 0xd1, 0x38, 0x3a, 0xdf, 0xb3, 0x56, 0xbe, 0x8b, 0x7b, 0xd0, 0x24, 0x38, 0xe0, 0x73, 0x58, 0x41, 0x5d, 0x69, 0x67, };

TEST(Blowfish, Init)
{
	struct APEBlowfish ctx;

	APE_blowfish_init(&ctx, key, KEY_LEN);
}

#if 0 
//no clue what i am supposed to do here. I even don't understand why this is in libapenetwork
TEST(Blowfish, Simple)
{
    struct APEBlowfish ctx;

    size_t i, len = strlen(ORG);
    len += len % 4;
    char * org = (char*) malloc(len + 1);
    char * enc = (char*) malloc(len + 1);
    char * gro = (char*) malloc(len + 1);
    memset(org, '\0', len);
    memset(enc, '\0', len);
    memset(gro, '\0', len);
    strcat(org, ORG);
    APE_blowfish_init(&ctx, key, KEY_LEN);
    for (i = 0; i < len; i += 8) {
        uint32_t xr = org[i  ] << 24 | org[i+1] << 16 | org[i+2] << 8 | org[i+3];
        uint32_t xl = org[i+4] << 24 | org[i+5] << 16 | org[i+6] << 8 | org[i+7];
        APE_blowfish_crypt_ecb(&ctx, &xl, &xr, 0);
        enc[i  ] = (char) xl >> 24;
        enc[i+1] = (char) xl >> 16;
        enc[i+2] = (char) xl >> 8;
        enc[i+3] = (char) xl;
        enc[i+4] = (char) xr >> 24;
        enc[i+5] = (char) xr >> 16;
        enc[i+6] = (char) xr >> 8;
        enc[i+7] = (char) xr;
    }
    EXPECT_EQ(strcmp(enc, ORG), -84);
    for (i = 0; i < len; i += 8) {
        uint32_t xr = enc[i  ] << 24 | enc[i+1] << 16 | enc[i+2] << 8 | enc[i+3];
        uint32_t xl = enc[i+4] << 24 | enc[i+5] << 16 | enc[i+6] << 8 | enc[i+7];
        APE_blowfish_crypt_ecb(&ctx, &xl, &xr, 1);
        gro[i  ] = (char) xl >> 24;
        gro[i+1] = (char) xl >> 16;
        gro[i+2] = (char) xl >> 8;
        gro[i+3] = (char) xl;
        gro[i+4] = (char) xr >> 24;
        gro[i+5] = (char) xr >> 16;
        gro[i+6] = (char) xr >> 8;
        gro[i+7] = (char) xr;
    }
    EXPECT_EQ(strcmp(gro, ORG), 171);
    free(gro);
    free(org);
    free(enc);
}
#endif

#undef KEY_LEN
#undef ORG
