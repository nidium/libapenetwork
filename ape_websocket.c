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

void ape_ws_init(websocket_state *state)
{
    state->socket = NULL;
    state->step    = WS_STEP_START;
    state->offset  = 0;
    state->data    = NULL;
    state->error   = 0;
    state->key.pos = 0;
    state->close_sent = 0;

    state->frame_payload.start  = 0;
    state->frame_payload.length = 0;
    state->frame_payload.extended_length = 0;
    state->data_pos  = 0;
    state->frame_pos = 0;
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

void ape_ws_write(ape_socket *socket_client, unsigned char *data,
    size_t len, int binary, ape_socket_data_autorelease data_type)
{
    unsigned char payload_head[32] = { 0x80 | (binary ? 0x02 : 0x01) };
    size_t payload_length = 0;

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

    PACK_TCP(socket_client->s.fd);
        APE_socket_write(socket_client, payload_head,
            payload_length, APE_DATA_STATIC);
        APE_socket_write(socket_client, data, len, data_type);
    FLUSH_TCP(socket_client->s.fd);
}

void ape_ws_close(websocket_state *state)
{
    if (state->close_sent)
        return;

    state->close_sent = 1;
    APE_socket_write(state->socket, (void *)"\x88\x00", 2, APE_DATA_STATIC);
}

void ape_ws_process_frame(websocket_state *websocket, const char *buf, size_t len)
{
    const buffer *buffer = &websocket->socket->data_in;
    unsigned char *pData;

    #if 1

    for (pData = (unsigned char *)&buffer->data[websocket->offset];
        websocket->offset < buffer->used; websocket->offset++, pData++) {

        switch (websocket->step) {
            case WS_STEP_KEY:
                /* Copy the xor key (32 bits) */
                websocket->key.val[websocket->key.pos] = *pData;

                if (++websocket->key.pos == 4) {
                    websocket->step = WS_STEP_DATA;
                              websocket->data_inkey = 0;
                }
                break;
            case WS_STEP_START:
                /* Contain fragmentation infos & opcode (+ reserved bits) */
                websocket->frame_payload.start = *pData;
                websocket->step = WS_STEP_LENGTH;
                break;
            case WS_STEP_LENGTH:
                /* Check for MASK bit */
                if (!(*pData & 0x80)) {
                    //websocket->step =
                    return;
                }
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
                        websocket->step = WS_STEP_KEY;
                        break;
                }
                break;
            case WS_STEP_SHORT_LENGTH:
                memcpy(((char *)&websocket->frame_payload)+(websocket->frame_pos),
                        pData, 1);

                if (websocket->frame_pos == 3) {

                    websocket->frame_payload.extended_length =
                        ntohs(websocket->frame_payload.short_length);

                    websocket->step = WS_STEP_KEY;
                }
                break;
            case WS_STEP_EXTENDED_LENGTH:
                memcpy(((char *)&websocket->frame_payload)+(websocket->frame_pos),
                        pData, 1);

                if (websocket->frame_pos == 9) {

                    websocket->frame_payload.extended_length =
                        ntohl(websocket->frame_payload.extended_length >> 32);

                    websocket->step = WS_STEP_KEY;
                }
                break;
            case WS_STEP_DATA:
                if (websocket->data_pos == 0) {
                    websocket->data_pos = websocket->offset;
                    /* TODO: zerocopy - uncypher data in place */
                    websocket->data = (unsigned char *)malloc(sizeof(char) *
                        websocket->frame_payload.extended_length + 1);
                }

                //*pData ^= websocket->key.val[websocket->data_inkey++ % 4];
                websocket->data[websocket->data_inkey] =
                    *pData ^ websocket->key.val[websocket->data_inkey % 4];

                websocket->data_inkey++;

                if (--websocket->frame_payload.extended_length == 0) {
                    unsigned char saved;
                    websocket->data[websocket->data_inkey] = '\0';
                    //websocket->data = &buffer->data[websocket->data_pos];
                    websocket->step = WS_STEP_START;
                    websocket->frame_pos = -1;
                    websocket->frame_payload.extended_length = 0;
                    websocket->data_pos = 0;
                    websocket->key.pos  = 0;

                    switch (websocket->frame_payload.start & 0x0F) {
                        case 0x8:
                        {
                            /*
                              Close frame
                              Reply by a close response
                              TODO : include close reason
                            */
                            char payload_head[2] = { 0x88, 0x00 };
                            if (!websocket->close_sent) {
                                APE_socket_write(websocket->socket,
                                    payload_head, 2, APE_DATA_STATIC);
                            }
                            APE_socket_shutdown(websocket->socket);
                            return;
                        }
                        case 0x9:
                        {
                            int body_length = /*&buffer->data[websocket->offset+1] - websocket->data;*/ 0;
                            char payload_head[2] = { 0x8a, body_length & 0x7F };

                            /* All control frames MUST be 125 bytes or less */
                            if (body_length > 125) {
                                payload_head[0] = 0x88;
                                payload_head[1] = 0x00;
                            //    sendbin(co->fd, payload_head, 2, 1, g_ape);
                                return;
                            }
                            //PACK_TCP(co->fd);
                          //  sendbin(co->fd, payload_head, 2, 0, g_ape);
                            if (body_length) {
                                //sendbin(co->fd, websocket->data, body_length, 0, g_ape);
                            }
                            //FLUSH_TCP(co->fd);
                            break;
                        }
                        case 0xA: /* Never called as long as we never ask for pong */
                            break;
                        case 0x1:
                            websocket->on_frame(websocket, websocket->data,
                                websocket->data_inkey, 0);
                            #if 0
                            /* Data frame */
                            saved = buffer->data[websocket->offset+1];
                            buffer->data[websocket->offset+1] = '\0';
                            //parser->onready(parser, g_ape);
                            buffer->data[websocket->offset+1] = saved;
                            #endif
                            break;
                        case 0x2:
                            websocket->on_frame(websocket, websocket->data,
                                websocket->data_inkey, 1);
                            break;
                        default:
                            break;
                    }

                    if (websocket->offset+1 == buffer->used) {
                        websocket->offset = 0;
                        //buffer->length = 0;
                        websocket->frame_pos = 0;
                        websocket->key.pos = 0;
                        return;
                    }
                }
                break;
            default:
                break;
        }
        websocket->frame_pos++;
    }
#endif
}

