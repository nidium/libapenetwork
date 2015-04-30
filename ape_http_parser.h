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

#ifndef __HTTP_PARSER_H_
#define __HTTP_PARSER_H_

#include <stdint.h>

/* Todo : check for endieness + aligned */
#define BYTES_GET(b) \
    *(uint32_t *) b == ((' ' << 24) | ('T' << 16) | ('E' << 8) | 'G')

#define BYTES_POST(b) \
    *(uint32_t *) b == (('T' << 24) | ('S' << 16) | ('O' << 8) | 'P')

typedef enum type {
    HTTP_PARSE_ERROR,
    HTTP_METHOD,
    HTTP_PATH_CHAR,
    HTTP_PATH_END,
    HTTP_QS_CHAR,
    HTTP_BODY_CHAR,
    HTTP_VERSION_MAJOR,
    HTTP_VERSION_MINOR,
    HTTP_HEADER_KEY,
    HTTP_HEADER_KEYC,
    HTTP_HEADER_VAL,
    HTTP_HEADER_VALC,
    HTTP_CL_VAL,
    HTTP_HEADER_END,
    HTTP_READY
} callback_type;


typedef enum states {
    GO,  /* start    */
    G1, /* GE */
    G2, /* GET */
    G3, /* GET  */
    P1, /* PO */
    P2, /* POS */
    P3, /* POST */
    P4, /* POST  */
    PT, /* path */
    H1, /* H */
    H2, /* HT */
    H3, /* HTT */
    H4, /* HTTP */
    H5, /* HTTP/ */
    H6, /* HTTP/[0-9] */
    H7, /* HTTP/[0-9]. */
    H8, /* HTTP/[0-9].[0-9] */
    EL, /* expect new line */
    ER, /* expect \n */
    HH, /* header key */
    HI, /* header value */
    C1, /* C */
    C2, /* O */
    C3, /* N */
    C4, /* T */
    C5, /* E */
    C6, /* N */
    C7, /* T */
    C8, /* - */
    C9, /* L */
    CA, /* E */
    CB, /* N */
    CC, /* G */
    CD, /* T */
    CE, /* H */
    CF, /* : */
    CG, /* space */
    CV, /* content-length value */
    E1, /* first hex in % path */
    E2, /* second hex in % path */
    FI, /* header is about to finish */
    BT, /* Body char */
    R1, /* H response */
    R2, /* HT response */
    R3, /* HTT response */
    R5, /* HTTP/ response */
    R6, /* HTTP/1 response */
    R7, /* HTTP/1. response */
    R8, /* HTTP/1.1 response */
    R9, /* HTTP/1.1 space */
    RN, /* HTTP/1.1 response code */
    RD, /* response description */
    AA, /* catchall */
    NR_STATES
} parser_state;

typedef int (*HTTP_parser_callback)(void **ctx, callback_type type,
        int value, uint32_t step);

typedef struct _http_parser {
    HTTP_parser_callback callback;  /* user callback function */
    void *ctx[2];              /* user defined */
    uint32_t rx;            /* flag (32bit) (pass through states) */
    uint32_t step;          /* char number */
    int32_t cl;            /* content-length */ /* TODO : store cl in rx */
    uint16_t rcode;         /* response code */
    parser_state state;     /* state */
} http_parser;


typedef enum methods {
    HTTP_GET,
    HTTP_POST
} http_method_t;

#ifdef __cplusplus
extern "C" {
#endif

int parse_http_char(http_parser *parser, const unsigned char c);

#ifdef __cplusplus
}
#endif

#define HTTP_PARSER_RESET(p) \
    do { \
        (p)->state = GO; \
        (p)->step   = 0; \
        (p)->cl = -1; \
        (p)->callback = NULL; \
        (p)->rx = 0; \
        (p)->rcode = 0; \
        (p)->ctx[0] = NULL; \
        (p)->ctx[1] = NULL; \
    } while(0) \

#endif

