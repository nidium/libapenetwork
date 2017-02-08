/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_buffer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "ape_common.h"

#define ZBUF_BUFSIZE 1024


#if APE_USE_ZLIB
struct gztrailer {
    uint32_t crc32;
    uint32_t zlen;
};

static void *zbuffer_allocate(void *opaque, uint items, uint size)
{
    APE_DEBUG("libapenetwork", "[Buffer] Allocate memory from zlib %d\n", items * size);
    return malloc(items * size);
}

static void zbuffer_adjust_outbuf(buffer *b)
{
    b->zbuf->zstream.avail_out = b->size - b->used;
    b->zbuf->zstream.next_out  = b->data + b->used;
}

static void zbuffer_prepapre_buf(buffer *b, size_t input_size)
{
    if (!b->zbuf) {
        return;
    }

    /* No pending data, don't need a buffer */
    if (!b->zbuf->zstream.avail_in) {
        return;
    }

    ssize_t remaining = b->zbuf->buf_size - b->zbuf->zstream.avail_in;

    if (remaining >= (ssize_t)input_size) {
        return;
    }

    size_t bufsize = ape_max(ZBUF_BUFSIZE, b->zbuf->buf_size);

    while (bufsize - b->zbuf->zstream.avail_in < input_size) {
        bufsize <<= 1;
    }

    APE_DEBUG("libapenetwork", "[Buffer] zbuf prepared for size %ld\n", bufsize);

    b->zbuf->buf_size = bufsize;

    if (b->zbuf->buf) {
        b->zbuf->buf = realloc(b->zbuf->buf, bufsize);
        APE_DEBUG("libapenetwork", "[Buffer] Realloc input buffer for size %ld\n", bufsize);
    } else {
        b->zbuf->buf = malloc(bufsize);
        APE_DEBUG("libapenetwork", "[Buffer] Alloc input buffer for size %ld\n", bufsize);
    }

    b->zbuf->zstream.avail_out = bufsize - b->zbuf->zstream.avail_in;
}

void buffer_set_gzip(buffer *b)
{
    if (b->zbuf) {
        return;
    }

    /* Reset buffer */
    b->used = 0;

    b->zbuf = malloc(sizeof(zbuffer));
    memset(b->zbuf, 0, sizeof(zbuffer));

    zbuffer *zbuf = b->zbuf;

    zbuf->zstream.zalloc  = zbuffer_allocate;
    zbuf->zstream.next_in = NULL;

    int rc = deflateInit2(&zbuf->zstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                          -15, 8, Z_DEFAULT_STRATEGY);

    if (rc != Z_OK) {
        APE_ERROR("libapenetwork", "[Buffer] Failed to init zlib\n");
        return;
    }

    zbuf->zstream.avail_in = 0;

    zbuffer_adjust_outbuf(b);

    zbuf->crc32 = crc32(0, Z_NULL, 0);
}

static void buffer_gzip_reset(buffer *b)
{
    deflateReset(&b->zbuf->zstream);
    if (b->zbuf->buf) {
        free(b->zbuf->buf);
        b->zbuf->buf      = NULL;
        b->zbuf->buf_size = 0;
    }
    b->zbuf->crc32 = 0;
}

#endif

void buffer_init(buffer *b)
{
    memset(b, 0, sizeof(buffer));
}


buffer *buffer_new(size_t size)
{
    buffer *b;

    b = malloc(sizeof(*b));
    buffer_init(b);

    /* TODO: removing a malloc by making b->data[] the last struct elem */

    if ((b->size = size) > 0) {
        b->data = malloc(size * sizeof(char));
    } else {
        b->size = 0;
    }
#if APE_USE_ZLIB
    b->zbuf = NULL;
#endif
    return b;
}

unsigned char *buffer_data(buffer *b, int *len)
{
#if APE_USE_ZLIB
    if (!b->zbuf) {
        *len = b->used;
        return b->data;
    }

    int ret = Z_OK;
    *len    = 0;

    if (b->zbuf->zstream.next_out == NULL) {
        return NULL;
    }

    while (ret == Z_OK || ret == Z_BUF_ERROR) {
        ret = deflate(&b->zbuf->zstream, Z_FINISH);
        if (ret == Z_STREAM_END) {
            b->used = b->zbuf->zstream.total_out;
            *len = b->used;
            buffer_gzip_reset(b);

            return b->data;
        } else if (ret == Z_BUF_ERROR) {
            b->used = b->zbuf->zstream.total_out;
            buffer_prepare(b, ZBUF_BUFSIZE);
        } else if (ret == Z_STREAM_ERROR) {
            APE_ERROR("libapenetwork", "[Buffer] Gzip stream error\n");

            return NULL;
        }
    }
#else
    *len = b->used;
    return b->data;
#endif
    return NULL;
}

void buffer_delete(buffer *b)
{
    if (b->data != NULL) {
        free(b->data);
        b->data = NULL;
        b->used = 0;
        b->size = 0;
    }
#if APE_USE_ZLIB
    if (b->zbuf) {
        if (b->zbuf->buf) {
            free(b->zbuf->buf);
        }
        deflateEnd(&b->zbuf->zstream);
    }
#endif
}

void buffer_destroy(buffer *b)
{
    if (b != NULL) {
        buffer_delete(b);
        free(b);
    }
}

void buffer_prepare(buffer *b, size_t size)
{
    if (b->size == 0) {
        b->size = size;
        b->used = 0;
        b->data = malloc(sizeof(char) * b->size);
    } else if (b->used + size > b->size) {
        if (size == 0) {
            size = 1;
        }
        b->size += size;
        b->data = realloc(b->data, sizeof(char) * b->size);
    }
#if APE_USE_ZLIB
    if (b->zbuf) {
        zbuffer_adjust_outbuf(b);
    }
#endif
}

static void buffer_prepare_for(buffer *b, size_t size, size_t forsize)
{
    if (b->size == 0) {
        b->size = size;
        b->used = 0;
        b->data = malloc(sizeof(char) * b->size);
    } else if (b->used + forsize > b->size) {
        if (size == 0) {
            size = 1;
        }
        b->size += size;
        b->data = realloc(b->data, sizeof(char) * b->size);
    }
#if APE_USE_ZLIB
    if (b->zbuf) {
        zbuffer_adjust_outbuf(b);
    }
#endif
}

void buffer_append_data(buffer *b, const unsigned char *data, size_t size)
{
    if (!size) {
        return;
    }
#if APE_USE_ZLIB
    if (b->zbuf) {
        buffer_prepare(b, deflateBound(&b->zbuf->zstream,
                                       b->zbuf->zstream.avail_in + size));

        zbuffer_prepapre_buf(b, size);
        int flush = b->zbuf->flush ? Z_FINISH : Z_NO_FLUSH;

        if (b->zbuf->zstream.next_in) {
            /* Pending data already in the in buffer */
            memcpy(b->zbuf->buf + b->zbuf->zstream.avail_in, data, size);
        } else {
            /* Nothing awaiting, read directly from the data */
            b->zbuf->zstream.next_in = (unsigned char *)data;
        }

        int consumed = b->zbuf->zstream.avail_in + size;
        int outsize  = b->zbuf->zstream.avail_out;

        b->zbuf->zstream.avail_in += size;
        b->zbuf->crc32 = crc32(b->zbuf->crc32, data, size);

        int ret = deflate(&b->zbuf->zstream, flush);

        b->used += outsize - b->zbuf->zstream.avail_out;

        consumed -= b->zbuf->zstream.avail_in;

        if (b->zbuf->zstream.avail_in == 0) {
            /* All input processed */
            b->zbuf->zstream.next_in = NULL;
        }

        if (ret == Z_OK && b->zbuf->zstream.avail_in) {
            zbuffer_prepapre_buf(b, b->zbuf->zstream.avail_in);
            memmove(b->zbuf->buf, b->zbuf->zstream.next_in, size - consumed);
            b->zbuf->zstream.next_in = b->zbuf->buf;

        } else if (ret == Z_STREAM_ERROR || ret == Z_BUF_ERROR) {
            APE_ERROR("libapenetwork", "[Buffer] Got an error.%d\n", ret);
        } else {
            if (ret == Z_STREAM_END) {
                deflateReset(&b->zbuf->zstream);
            }
        }

    } else {
#endif
        buffer_prepare(b, size + 1);
        memcpy(b->data + b->used, data, size);
        b->data[b->used + size] = '\0';
        b->used += size;
#if APE_USE_ZLIB
    }
#endif
}

void buffer_append_data_tolower(buffer *b, const unsigned char *data,
                                size_t size)
{
    buffer_prepare(b, size + 1);
    unsigned i;

    for (i = 0; i < size; i++) {
        b->data[b->used + i] = tolower(data[i]);
    }

    b->data[b->used + size] = '\0';
    b->used += size;
}

void buffer_append_char(buffer *b, const unsigned char data)
{
    buffer_prepare_for(b, 1024, 1);
    b->data[b->used] = data;
    b->used++;
}

void buffer_append_string(buffer *b, const char *string)
{
    buffer_append_string_n(b, string, strlen(string));
}

void buffer_append_string_n(buffer *b, const char *string, size_t length)
{
    buffer_prepare(b, length + 1);

    memcpy(b->data + b->used, string, length);
    b->used += length;
    b->data[b->used] = '\0';
}

void buffer_camelify(buffer *b)
{
    unsigned char *pSource, pchar;

    for (pSource = b->data, pchar = '-'; *pSource; pSource++) {
        if (pchar == '-') {
            *pSource = toupper(*pSource);
        }
        pchar = *pSource;
    }
}

/* taken from PHP 5.3 */
buffer *buffer_to_buffer_utf8(buffer *b)
{
    int pos          = b->used;
    unsigned char *s = b->data;
    unsigned int c;

    buffer *newb = buffer_new(b->used * 4 + 1);

    while (pos > 0) {
        c = (unsigned short)(unsigned char)*s;

        if (c < 0x80) {
            newb->data[newb->used++] = (char)c;
        } else if (c < 0x800) {
            newb->data[newb->used++] = (0xc0 | (c >> 6));
            newb->data[newb->used++] = (0x80 | (c & 0x3f));
        } else if (c < 0x10000) {
            newb->data[newb->used++] = (0xe0 | (c >> 12));
            newb->data[newb->used++] = (0xc0 | ((c >> 6) & 0x3f));
            newb->data[newb->used++] = (0x80 | (c & 0x3f));
        } else if (c < 0x200000) {
            newb->data[newb->used++] = (0xf0 | (c >> 18));
            newb->data[newb->used++] = (0xe0 | ((c >> 12) & 0x3f));
            newb->data[newb->used++] = (0xc0 | ((c >> 6) & 0x3f));
            newb->data[newb->used++] = (0x80 | (c & 0x3f));
        }
        pos--;
        s++;
    }
    newb->data[newb->used] = '\0';

    if (newb->size > newb->used + 1) {
        newb->size = newb->used + 1;
        newb->data = realloc(newb->data, newb->size);
    }

    return newb;
}

/* taken from PHP 5.3 (sry) */
buffer *buffer_utf8_to_buffer(buffer *b)
{
    int pos          = b->used;
    unsigned char *s = b->data;
    unsigned int c;

    buffer *newb = buffer_new(b->used + 1);

    while (pos > 0) {
        c = (unsigned char)(*s);

        if (c >= 0xf0) { /* four bytes encoded, 21 bits */
            if (pos - 4 >= 0) {
                c = ((s[0] & 7) << 18) | ((s[1] & 63) << 12)
                    | ((s[2] & 63) << 6) | (s[3] & 63);
            } else {
                c = '?';
            }
            s += 4;
            pos -= 4;
        } else if (c >= 0xe0) { /* three bytes encoded, 16 bits */
            if (pos - 3 >= 0) {
                c = ((s[0] & 63) << 12) | ((s[1] & 63) << 6) | (s[2] & 63);
            } else {
                c = '?';
            }
            s += 3;
            pos -= 3;
        } else if (c >= 0xc0) { /* two bytes encoded, 11 bits */
            if (pos - 2 >= 0) {
                c = ((s[0] & 63) << 6) | (s[1] & 63);
            } else {
                c = '?';
            }
            s += 2;
            pos -= 2;
        } else {
            s++;
            pos--;
        }
        newb->data[newb->used++] = (char)(c > 0xff ? '?' : c);
    }

    newb->data[newb->used] = '\0';

    if (newb->size > newb->used + 1) {
        newb->size = newb->used + 1;
        newb->data = realloc(newb->data, newb->size);
    }

    return newb;
}

