/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef __APE_BUFFER_H_
#define __APE_BUFFER_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

#ifndef _WIN32
#define APE_USE_ZLIB 1
#include <zlib.h>

typedef struct {
    z_stream zstream;
    int crc32;

    unsigned char *buf;
    size_t buf_size;
    size_t pending_size;
    int flush : 1;
} zbuffer;
#endif

typedef struct {
    unsigned char *data;

    size_t size;
    size_t used;

    uint32_t pos;
#if APE_USE_ZLIB
    zbuffer *zbuf;
#endif
} buffer;
/*
    static u_char  gzheader[10] =
                               { 0x1f, 0x8b, Z_DEFLATED, 0, 0, 0, 0, 0, 0, 3 };
*/


#ifdef __cplusplus
extern "C" {
#endif

void buffer_init(buffer *b);
buffer *buffer_new(size_t size);

#if APE_USE_ZLIB
void buffer_set_gzip(buffer *b);
#endif

unsigned char *buffer_data(buffer *b, int *len);

void buffer_delete(buffer *b);
void buffer_destroy(buffer *b);
void buffer_prepare(buffer *b, size_t size);
void buffer_append_data(buffer *b, const unsigned char *data, size_t size);
void buffer_append_data_tolower(buffer *b, const unsigned char *data,
                                size_t size);
void buffer_append_char(buffer *b, const unsigned char data);
void buffer_append_string(buffer *b, const char *string);
void buffer_append_string_n(buffer *b, const char *string, size_t length);
buffer *buffer_to_buffer_utf8(buffer *b);
buffer *buffer_utf8_to_buffer(buffer *b);

void buffer_camelify(buffer *b);

#ifdef __cplusplus
}
#endif

#endif

