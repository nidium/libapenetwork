/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_websocket.h"
#include "ape_sha1.h"
#include "ape_base64.h"

#include "ape_socket.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <arpa/inet.h>
#endif

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

enum {
    WS_CTRL_FRAME_CLOSE = 0x8,
    WS_CTRL_FRAME_PING  = 0x9,
    WS_CTRL_FRAME_PONG  = 0xA
};

static void ape_ws_send_frame(websocket_state *state, int opcode);

static uint32_t ape_random32(ape_global *ape)
{
    uint32_t ret;

    read(ape->urandom_fd, &ret, sizeof(uint32_t));

    return ret;
}

void ape_ws_init(websocket_state *state, int isclient)
{
    state->socket = NULL;
    state->step    = WS_STEP_START;
    state->data    = NULL;
    state->error   = 0;
    state->key.pos = 0;
    state->close_sent = 0;
    state->is_client = isclient;

    state->frame_payload.start  = 0;
    state->frame_payload.extended_length = 0;
    state->frame_pos = 0;
    state->data_inkey = 0;
}

char *ape_ws_compute_key(const char *key, unsigned int key_len)
{
    unsigned char digest[20];
    char out[128];
    char *b64;
    
    if (key_len > 32) {
        return NULL;
    }
    
    memcpy(out, key, key_len);
    memcpy(out+key_len, WS_GUID, sizeof(WS_GUID)-1);
    
    sha1_csum((unsigned char *)out, (sizeof(WS_GUID)-1)+key_len, digest);
    
    b64 = base64_encode(digest, 20);
    
    return b64; /* must be released */
}

void ape_ws_write(websocket_state *state, unsigned char *data,
    size_t len, int binary, ape_socket_data_autorelease data_type)
{
    unsigned char payload_head[32] = { 0x80 | (binary ? 0x02 : 0x01) };
    size_t payload_length = 0;
    ape_socket *socket_client = state->socket;

    if (len <= 125) {
        payload_head[1] = (unsigned char)len & 0x7F;
        
        payload_length = 2;
    } else if (len <= 65535) {
        unsigned short int s = htons(len);
        payload_head[1] = 126;
        memcpy(&payload_head[2], &s, 2);
        
        payload_length = 4;
        
    } else if (len <= 0xFFFFFFFF) {
        unsigned int s = htonl(len);
        payload_head[1] = 127;
        payload_head[2] = 0;
        payload_head[3] = 0;
        payload_head[4] = 0;
        payload_head[5] = 0;
        
        memcpy(&payload_head[6], &s, 4);
        
        payload_length = 10;
    }

    if (state->is_client) {
         /* MASK bit */
        payload_head[1] |= 0x80;
    }

    PACK_TCP(socket_client->s.fd);
        APE_socket_write(socket_client, payload_head,
            payload_length, APE_DATA_COPY);

        if (state->is_client /* Masking */) {
            uint32_t cipherKey = ape_random32(socket_client->ape);
            /* in-place ciphering */
            for (int i = 0; i < len; i++) {
                data[i] ^= ((uint8_t *)&cipherKey)[i%4];
            }

            APE_socket_write(socket_client, &cipherKey, sizeof(uint32_t), APE_DATA_COPY);
        }
        APE_socket_write(socket_client, data, len, data_type);
    FLUSH_TCP(socket_client->s.fd);
}

void ape_ws_close(websocket_state *state)
{
    ape_ws_send_frame(state, WS_CTRL_FRAME_CLOSE);
}

void ape_ws_ping(websocket_state *state)
{
    ape_ws_send_frame(state, WS_CTRL_FRAME_PING);
}

void ape_ws_pong(websocket_state *state)
{
    ape_ws_send_frame(state, WS_CTRL_FRAME_PONG);
}

static void ape_ws_send_frame(websocket_state *state, int opcode)
{
    if (state->close_sent) {
        return;
    }

    /* 0x80 == FIN frame (no continuation frame) */
    unsigned char payload_head[2] = { 0x80 | opcode, 0x00 };

    if (state->is_client) {
        payload_head[1] |= 0x80;
    }

    if (opcode == WS_CTRL_FRAME_CLOSE) {
        state->close_sent = 1;
    }

    APE_socket_write(state->socket, (void *)payload_head, 2, APE_DATA_COPY);

    if (state->is_client /* Masking */) {
        uint32_t cipherKey = ape_random32(state->socket->ape);

        APE_socket_write(state->socket, &cipherKey, sizeof(uint32_t), APE_DATA_COPY);
    }
}

static void ape_ws_reset_frame_state(websocket_state *websocket)
{
    websocket->step                          = WS_STEP_START;
    websocket->frame_pos                     = 0;
    websocket->frame_payload.extended_length = 0;
    websocket->key.pos                       = 0;

    if (websocket->data) {
        free(websocket->data);
        websocket->data = NULL;
    }
}

static int ape_ws_process_end_message(websocket_state *websocket)
{
    unsigned char opcode = websocket->frame_payload.start & 0x0F;
    int retval = 1;

    switch (opcode) {
        case 0x8: /* Close frame */
            ape_ws_close(websocket);
            retval = 0; /* Don't process anything more */
            break;
        case 0x9: /* Ping frame */
            ape_ws_pong(websocket);
            break;
        case 0xA: /* Pong frame */
            break;
        case 0x1: /* ASCII frame */
             websocket->on_frame(websocket, websocket->data,
                websocket->data_inkey, 0);           
            break;
        case 0x2: /* Binary frame */
            websocket->on_frame(websocket, websocket->data,
                websocket->data_inkey, 1);
            break;
        default:
            printf("Got an unknown frame with opcode %.2x\n", opcode);
            break;
    }

    ape_ws_reset_frame_state(websocket);

    return retval;
}

void ape_ws_process_frame(websocket_state *websocket, const char *buf, size_t len)
{
    const buffer *buffer = &websocket->socket->data_in;
    unsigned char *pData;
    int pos;

    for (pData = buffer->data, pos = 0; pos < buffer->used; pos++, pData++) {

        switch(websocket->step) {
            case WS_STEP_KEY:
                /* Copy the xor key (32 bits) */
                websocket->key.val[websocket->key.pos] = *pData;

                if (++websocket->key.pos == 4) {
                    websocket->step = WS_STEP_DATA;
                    websocket->data_inkey = 0;

                    if (!websocket->frame_payload.extended_length) { /* "no application data" */

                        if (!ape_ws_process_end_message(websocket)) {
                            return;
                        }

                        websocket->frame_pos = -1;
                    }
                }
                break;
            case WS_STEP_START:
                /* Contain fragmentation infos & opcode (+ reserved bits) */
                websocket->frame_payload.start = *pData;
                websocket->step = WS_STEP_LENGTH;
                websocket->data_inkey = 0;

                break;
            case WS_STEP_LENGTH:
                /* Check for MASK bit */
                websocket->mask = (*pData & 0x80);
                    
                switch (*pData & 0x7F) { /* 7bit length */
                    case 126:
                        /* Following 16bit are length */
                        websocket->step = WS_STEP_SHORT_LENGTH;
                        break;
                    case 127:
                        /* Following 64bit are length */
                        websocket->step = WS_STEP_EXTENDED_LENGTH;
                        break;
                    default:
                        /* We have the actual length */
                        websocket->frame_payload.extended_length = *pData & 0x7F;
                        websocket->step = websocket->mask ? WS_STEP_KEY : WS_STEP_DATA;

                         /* "no application data" */
                        if (!websocket->mask && !websocket->frame_payload.extended_length) {

                            if (!ape_ws_process_end_message(websocket)) {
                                return;
                            }

                            websocket->frame_pos = -1;
                        }

                        break;
                }
                break;
            case WS_STEP_SHORT_LENGTH:
                memcpy(((char *)&websocket->frame_payload)+(websocket->frame_pos), 
                        pData, 1);

                if (websocket->frame_pos == 3) {

                    websocket->frame_payload.extended_length =
                        ntohs(websocket->frame_payload.short_length);

                    websocket->step = websocket->mask ? WS_STEP_KEY : WS_STEP_DATA;
                }
                break;
            case WS_STEP_EXTENDED_LENGTH:
                memcpy(((char *)&websocket->frame_payload)+(websocket->frame_pos),
                        pData, 1);

                if (websocket->frame_pos == 9) {

                    websocket->frame_payload.extended_length =
                        ntohl(websocket->frame_payload.extended_length >> 32);

                    websocket->step = websocket->mask ? WS_STEP_KEY : WS_STEP_DATA;
                }
                break;
            case WS_STEP_DATA:
                if (websocket->data == NULL) {
                    websocket->data = (unsigned char *)malloc(sizeof(char) *
                        websocket->frame_payload.extended_length + 1);
                }
                
                websocket->data[websocket->data_inkey] = websocket->mask ?
                            *pData ^ websocket->key.val[websocket->data_inkey % 4] :
                            *pData;

                websocket->data_inkey++;
                
                if (--websocket->frame_payload.extended_length == 0) {
                    websocket->data[websocket->data_inkey] = '\0';

                    if (!ape_ws_process_end_message(websocket)) {
                        return;
                    }

                    websocket->frame_pos = -1;

                }
                break;
            default:
                break;
        }
        websocket->frame_pos++;

    }
}

