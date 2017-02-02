/*
 *  FIPS-180-1 compliant SHA-1 implementation
 *
 *  Copyright (C) 2003-2006  Christophe Devine
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License, version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */

#ifndef _SHA1_H
#define _SHA1_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STD_TYPES
#define _STD_TYPES

#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long int

#endif

typedef struct {
    ulong total[2];
    ulong state[5];
    uchar buffer[64];
} sha1_context;

/*
 * Core SHA-1 functions
 */
void sha1_starts(sha1_context *ctx);
void sha1_update(sha1_context *ctx, uchar *input, uint length);
void sha1_finish(sha1_context *ctx, uchar digest[20]);

/*
 * Output SHA-1(file contents), returns 0 if successful.
 */
int sha1_file(char *filename, uchar digest[20]);

/*
 * Output SHA-1(buf)
 */
void sha1_csum(uchar *buf, uint buflen, uchar digest[20]);

/*
 * Output HMAC-SHA-1(key, buf)
 */
void sha1_hmac(uchar *key, uint keylen, uchar *buf, uint buflen,
               uchar digest[20]);


#ifdef __cplusplus
}
#endif
#endif /* sha1.h */

