/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_BASE64_H
#define __APE_BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

int base64_decode(unsigned char *out, const char *in, int out_length);

void base64_encode_b(unsigned char *src, char *dst, int len);
void base64_encode_b_safe(unsigned char *src, char *dst, int len, int safe);
char *base64_encode(unsigned char *src, int len);
char *base64_encode_safe(unsigned char *src, int len);

#ifdef __cplusplus
}
#endif

#endif

