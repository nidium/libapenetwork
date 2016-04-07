/*
 * Blowfish algorithm
 * Copyright (c) 2012 Samuel Pitoiset
 *
 * loosely based on Paul Kocher's implementation
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __APE_BLOWFISH_H
#define __APE_BLOWFISH_H

#include <stdint.h>

#define APE_BF_ROUNDS 16

typedef struct APEBlowfish {
    uint32_t p[APE_BF_ROUNDS + 2];
    uint32_t s[4][256];
} APEBlowfish;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize an APEBlowfish context.
 *
 * @param ctx an APEBlowfish context
 * @param key a key
 * @param key_len length of the key
 */
void APE_blowfish_init(struct APEBlowfish *ctx, const uint8_t *key, int key_len);

/**
 * Encrypt or decrypt a buffer using a previously initialized context.
 *
 * @param ctx an APEBlowfish context
 * @param xl left four bytes halves of input to be encrypted
 * @param xr right four bytes halves of input to be encrypted
 * @param decrypt 0 for encryption, 1 for decryption
 */
void APE_blowfish_crypt_ecb(struct APEBlowfish *ctx, uint32_t *xl, uint32_t *xr,
                           int decrypt);


#ifdef __cplusplus
}
#endif

#endif /* __APE_BLOWFISH_H */
