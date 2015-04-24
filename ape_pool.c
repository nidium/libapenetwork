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

#include "ape_pool.h"
#include <stdlib.h>
#include <stdio.h>

ape_pool_t *ape_new_pool(size_t size, const size_t n)
{
    size_t i;
    const size_t stop = n - 1;
    ape_pool_t * pool, *current;

     if (size == 0) {
        size = sizeof(ape_pool_t);
    } else if ( n == 0 ) {
        return NULL;
    } else if (size < sizeof(ape_pool_t) ) {
        return NULL;
    }
    
    current = pool = malloc(size * n);
    pool->prev = NULL;
    i = 0;
    while(current != NULL) {
        current->ptr.data  = NULL;
        current->flags = 0;
        if (i == stop) {
            current = current->next = NULL;
        } else {
            i++;
            current->next = (ape_pool_t*) (( (char *)&pool[0]) + (i * size));
            current->next->prev = current;
            current = current->next;
        }
    }
    pool->flags = APE_POOL_ALLOC;

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

    list->head  = pool;
    list->current   = pool;
    list->queue     = (ape_pool_t*)(((char *)&pool[0])+((n-1)*size));
    list->size = size;
}

ape_pool_t *ape_pool_head_to_queue(ape_pool_list_t *list)
{
    ape_pool_t *head = list->head;

    list->head = head->next;
    list->queue->next = head;
    list->queue = head;
    head->next = NULL;

    return list->head;
}

ape_pool_t *ape_pool_head_to_current(ape_pool_list_t *list)
{
    ape_pool_t *head = list->head;

    if (head == list->current) {
        return head;
    }

    list->head = head->next;
    head->next = list->current->next;
    list->current->next = head;
    list->current = head;

    if (head->next == NULL) {
        list->queue = head;
    }

    return list->head;

}

ape_pool_t *ape_grow_pool(ape_pool_list_t *list, size_t n)
{
    ape_pool_t *pool;

    pool = ape_new_pool(list->size, n);
    list->queue->next = pool;
    pool->prev = list->queue;

    list->queue = (ape_pool_t *)(((char *)&pool[0])+((n-1)*list->size));

    return pool;
}

void ape_destroy_pool_ordered(ape_pool_t *pool, ape_pool_clean_callback cleaner, void *ctx)
{
    ape_pool_t *tPool = NULL;

    while (pool != NULL) {
        if (cleaner != NULL) {
            cleaner(pool, ctx);
        }
        if (pool->flags & APE_POOL_ALLOC) {
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
    ape_pool_t *tPool = NULL, *fPool = NULL;

    while (pool != NULL) {
        /* TODO : callback ? (cleaner) */
        if (pool->flags & APE_POOL_ALLOC) {
            if (fPool == NULL) {
                fPool = pool;
            }
            if (tPool != NULL) {
                fPool->next = pool->next;
                pool->next = tPool;
                tPool = pool;
                pool = fPool->next;
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
    list->current = list->current->next;
}

void ape_pool_rewind(ape_pool_list_t *list)
{
    list->current = list->head;
}

