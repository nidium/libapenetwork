/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
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
    struct _ape_pool *prev;
    uint32_t flags;
} ape_pool_t;

typedef struct _ape_pool_list {
    ape_pool_t *head;
    ape_pool_t *queue;
    ape_pool_t *current;

    size_t size;
} ape_pool_list_t;

typedef void (*ape_pool_clean_callback)(ape_pool_t *, void *ctx);

#ifdef __cplusplus
extern "C" {
#endif

ape_pool_t *ape_new_pool(size_t size, size_t n);
ape_pool_list_t *ape_new_pool_list(size_t size, size_t n);
ape_pool_t *ape_grow_pool(ape_pool_list_t *list, size_t n);
ape_pool_t *ape_pool_head_to_queue(ape_pool_list_t *list);
ape_pool_t *ape_pool_head_to_current(ape_pool_list_t *list);
void ape_pool_push(ape_pool_list_t *list, void *data);
void ape_pool_rewind(ape_pool_list_t *list);

void ape_init_pool_list(ape_pool_list_t *list, size_t size, size_t n);
void ape_destroy_pool(ape_pool_t *pool);
void ape_destroy_pool_with_cleaner(ape_pool_t *pool,
                                   ape_pool_clean_callback cleaner, void *ctx);
void ape_destroy_pool_ordered(ape_pool_t *pool, ape_pool_clean_callback cleaner,
                              void *ctx);
void ape_destroy_pool_list(ape_pool_list_t *list);
void ape_destroy_pool_list_ordered(ape_pool_list_t *list,
                                   ape_pool_clean_callback cleaner, void *ctx);
void ape_destroy_pool_list_with_cleaner(ape_pool_list_t *list,
                                        ape_pool_clean_callback cleaner,
                                        void *ctx);
#ifdef __cplusplus
}
#endif

#define APE_P_FOREACH(_list, _val)                                             \
    ape_pool_t *__pool_item = NULL;                                            \
    for (__pool_item = _list->head;                                            \
         __pool_item != NULL && (_val = __pool_item->ptr.data) != NULL;        \
         __pool_item = __pool_item->next)

#define APE_P_FOREACH_REVERSE(_list, _val)                                     \
    ape_pool_t *__pool_item = NULL;                                            \
    _val = NULL;                                                               \
    if (_list->current)                                                        \
        for (__pool_item = _list->current->prev;                               \
             __pool_item != NULL && (_val = __pool_item->ptr.data) != NULL;    \
             __pool_item = __pool_item->prev)

#endif

