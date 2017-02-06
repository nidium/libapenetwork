/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_pool.h"
#include <stdlib.h>
#include <stdio.h>

ape_pool_t *ape_new_pool(size_t size, size_t n)
{
    unsigned int i;

    if (size == 0) {
        size = sizeof(ape_pool_t);
    }

    ape_pool_t *pool = malloc(size * n), *current = NULL;
    pool->prev = NULL;

    for (i = 0; i < n; i++) {
        current = (ape_pool_t *)(((char *)&pool[0]) + (i * size));
        /* contiguous blocks */
        current->next
            = (i == n - 1 ? NULL : (ape_pool_t *)(((char *)&pool[0])
                                                  + ((i + 1) * size)));
        current->ptr.data = NULL;
        current->flags = (i == 0 ? APE_POOL_ALLOC : 0);
        if (current->next) {
            current->next->prev = current;
        }
    }

    return pool;
}

ape_pool_list_t *ape_new_pool_list(size_t size, size_t n)
{
    ape_pool_list_t *list = malloc(sizeof(ape_pool_list_t));

    ape_init_pool_list(list, size, n);

    return list;
}

void ape_init_pool_list(ape_pool_list_t *list, size_t size, size_t n)
{
    if (size == 0) {
        size = sizeof(ape_pool_t);
    }

    ape_pool_t *pool = ape_new_pool(size, n);

    list->head    = pool;
    list->current = pool;
    list->queue   = (ape_pool_t *)(((char *)&pool[0]) + ((n - 1) * size));
    list->size    = size;
}

ape_pool_t *ape_pool_head_to_queue(ape_pool_list_t *list)
{
    ape_pool_t *head = list->head;

    list->head        = head->next;
    list->queue->next = head;
    list->queue       = head;
    head->next        = NULL;

    return list->head;
}

ape_pool_t *ape_pool_head_to_current(ape_pool_list_t *list)
{
    ape_pool_t *head = list->head;

    if (head == list->current) {
        return head;
    }

    list->head          = head->next;
    head->next          = list->current->next;
    list->current->next = head;
    list->current       = head;

    if (head->next == NULL) {
        list->queue = head;
    }

    return list->head;
}

ape_pool_t *ape_grow_pool(ape_pool_list_t *list, size_t n)
{
    ape_pool_t *pool;

    pool              = ape_new_pool(list->size, n);
    list->queue->next = pool;
    pool->prev        = list->queue;

    list->queue = (ape_pool_t *)(((char *)&pool[0]) + ((n - 1) * list->size));

    return pool;
}

void ape_destroy_pool_ordered(ape_pool_t *pool, ape_pool_clean_callback cleaner,
                              void *ctx)
{
    ape_pool_t *tPool = NULL;

    while (pool != NULL) {
        if (cleaner != NULL) {
            cleaner(pool, ctx);
        }
        if (pool->flags & APE_POOL_ALLOC) {
            /*
                Free the previous block.
                We don't directly free the current block,
                because ->next could be part of the same allocation.
            */
            if (tPool != NULL) {
                free(tPool);
            }
            tPool = pool;
        }
        pool = pool->next;
    }

    if (tPool != NULL) {
        free(tPool);
    }
}

void ape_destroy_pool(ape_pool_t *pool)
{
    ape_destroy_pool_with_cleaner(pool, NULL, NULL);
}

void ape_destroy_pool_with_cleaner(ape_pool_t *pool,
                                   ape_pool_clean_callback cleaner, void *ctx)
{
    ape_pool_t *tPool = NULL, *fPool = NULL;

    while (pool != NULL) {
        if (cleaner != NULL) {
            cleaner(pool, ctx);
        }
        if (pool->flags & APE_POOL_ALLOC) {
            if (fPool == NULL) {
                fPool = pool;
            }
            if (tPool != NULL) {
                fPool->next = pool->next;
                pool->next  = tPool;
                tPool       = pool;
                pool        = fPool->next;
                continue;
            }
            tPool = pool;
        }
        pool = pool->next;
    }

    if (fPool) {
        fPool->next = NULL;
    }
    pool = tPool;

    while (pool != NULL && (pool->flags & APE_POOL_ALLOC)) {
        tPool = pool->next;
        free(pool);
        pool = tPool;
    }
}

void ape_destroy_pool_list(ape_pool_list_t *list)
{
    ape_destroy_pool(list->head);
    free(list);
}

void ape_destroy_pool_list_with_cleaner(ape_pool_list_t *list,
                                        ape_pool_clean_callback cleaner,
                                        void *ctx)
{
    ape_destroy_pool_with_cleaner(list->head, cleaner, ctx);
    free(list);
}

void ape_destroy_pool_list_ordered(ape_pool_list_t *list,
                                   ape_pool_clean_callback cleaner, void *ctx)
{
    ape_destroy_pool_ordered(list->head, cleaner, ctx);
    free(list);
}

void ape_pool_push(ape_pool_list_t *list, void *data)
{
    if (!list) {
        return;
    }

    if (list->current->next == NULL) {
        ape_grow_pool(list, 8);
    }

    list->current->ptr.data = data;
    list->current           = list->current->next;
}

void ape_pool_rewind(ape_pool_list_t *list)
{
    list->current = list->head;
}

