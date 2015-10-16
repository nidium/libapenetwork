/*
    APE Network Library
    Copyright (C) 2010-2014 Anthony Catel <paraboul@gmail.com>

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

#ifndef __APE_WEBSOCKET_H
#define __APE_WEBSOCKET_H

#include "common.h"
#include "ape_socket.h"

typedef enum {
    WS_STEP_KEY,
    WS_STEP_START,
    WS_STEP_LENGTH,
    WS_STEP_SHORT_LENGTH,
    WS_STEP_EXTENDED_LENGTH,
    WS_STEP_DATA,
    WS_STEP_END
} ws_payload_step;

typedef struct _websocket_state
{
    ape_socket *socket;

    unsigned char *data;
    void (*on_frame)(struct _websocket_state *,
        const unsigned char *, ssize_t, int binary);
    
    unsigned short int error;
    //ws_version version;
    
    struct {
        /* cypher key */
        unsigned char val[4];
        int pos;
    } key;
    
    #pragma pack(2)
    struct {
        unsigned char start;
        union {
            unsigned short short_length; /* 16 bit length */
            unsigned long long int extended_length; /* 64 bit length */
        };
    } frame_payload;
    #pragma pack()
    
    ws_payload_step step;

    int data_inkey;
    int frame_pos;
    int close_sent;
    int mask;
} websocket_state;

#ifdef __cplusplus
extern "C" {
#endif

void ape_ws_init(websocket_state *state);
void ape_ws_process_frame(websocket_state *websocket, const char *buf, size_t len);
char *ape_ws_compute_key(const char *key, unsigned int key_len);
void ape_ws_write(ape_socket *socket_client, unsigned char *data,
    size_t len, int binary,
    ape_socket_data_autorelease data_type, uint32_t *cipherKey);
    
void ape_ws_close(websocket_state *state);
void ape_ws_ping(websocket_state *state);

#ifdef __cplusplus
}
#endif

#define WEBSOCKET_HARDCODED_HEADERS "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"

#endif

