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

#ifndef __APE_BUFFER_H_
#define __APE_BUFFER_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <zlib.h>

typedef struct {
    z_stream zstream;
    int crc32;

    unsigned char *buf;
    size_t buf_size;
    size_t pending_size;
    int flush:1;
} zbuffer;

typedef struct {
    unsigned char *data;

    size_t size;
    size_t used;
    
    uint32_t pos;

    zbuffer *zbuf;
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
void buffer_set_gzip(buffer *b);
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

// vim: ts=4 sts=4 sw=4 et

