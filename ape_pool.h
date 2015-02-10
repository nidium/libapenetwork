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

#ifndef __APE_POOL_H
#define __APE_POOL_H

#include "ape_buffer.h"

#define APE_POOL_ALLOC 0x01
#define APE_POOL_ALL_FLAGS APE_POOL_ALLOC

typedef struct _ape_pool {
    union {
        void *data;
        int fd;
        buffer *buf;
    } ptr; /* public */
    struct _ape_pool *next;
    uint32_t flags;
} ape_pool_t;

typedef struct _ape_pool_list {
    ape_pool_t *head;
    ape_pool_t *queue;
    ape_pool_t *current;

    size_t size;
} ape_pool_list_t;

typedef void (*ape_pool_clean_callback)(ape_pool_t *);

#ifdef __cplusplus
extern "C" {
#endif

ape_pool_t *ape_new_pool(size_t size, size_t n);
ape_pool_list_t *ape_new_pool_list(size_t size, size_t n);
ape_pool_t *ape_grow_pool(ape_pool_list_t *list, size_t n);
ape_pool_t *ape_pool_head_to_queue(ape_pool_list_t *list);
ape_pool_t *ape_pool_head_to_current(ape_pool_list_t *list);
void ape_pool_push(ape_pool_list_t *list, void *data);

void ape_init_pool_list(ape_pool_list_t *list, size_t size, size_t n);
void ape_destroy_pool(ape_pool_t *pool);
void ape_destroy_pool_ordered(ape_pool_t *pool,
    ape_pool_clean_callback cleaner);
void ape_destroy_pool_list(ape_pool_list_t *list);
void ape_destroy_pool_list_ordered(ape_pool_list_t *list,
    ape_pool_clean_callback cleaner);
#ifdef __cplusplus
}
#endif


#endif

// vim: ts=4 sts=4 sw=4 et

