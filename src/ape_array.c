/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include "ape_array.h"
#include <string.h>


static void ape_array_clean_cb(ape_pool_t *item, void *ctx);

ape_array_t *ape_array_new(size_t n)
{
    ape_array_t *array;
    array = (ape_array_t *)ape_new_pool_list(sizeof(ape_array_item_t), n);

    return array;
}

ape_array_item_t *ape_array_lookup_item(ape_array_t *array, const char *key,
                                        int klen)
{
    buffer *k, *v;
    if (!array) {
        return NULL;
    }
    APE_A_FOREACH(array, k, v)
    {
        if (k->used == klen && memcmp(key, k->data, klen) == 0) {
            return __array_item;
        }
    }

    return NULL;
}

buffer *ape_array_lookup(ape_array_t *array, const char *key, int klen)
{
    buffer *k, *v;
    if (!array) {
        return NULL;
    }

    APE_A_FOREACH(array, k, v)
    {
        if (k->used == klen
            && strncasecmp(key, (const char *)k->data, klen) == 0) {
            return v;
        }
    }

    return NULL;
}

buffer *ape_array_lookup_cstr(ape_array_t *array, const char *key, int klen)
{
    buffer *k, *v;
    if (!array) {
        return NULL;
    }

    APE_A_FOREACH(array, k, v)
    {
        if (k->data[k->used - 1] == '\0' && k->used == klen + 1
            && strcasecmp(key, (const char *)k->data) == 0) {
            return v;
        }
    }

    return NULL;
}

void *ape_array_lookup_data(ape_array_t *array, const char *key, int klen)
{
    buffer *k, *v;
    if (!array) {
        return NULL;
    }
    APE_A_FOREACH(array, k, v)
    {
        if (k->used == klen
            && strncasecmp(key, (const char *)k->data, klen) == 0) {
            return __array_item->pool.ptr.data;
        }
    }

    return NULL;
}

void ape_array_delete(ape_array_t *array, const char *key, int klen)
{
    if (!array) {
        return;
    }
    ape_array_item_t *item = ape_array_lookup_item(array, key, klen);

    if (item != NULL) {
        item->pool.flags &= ~APE_ARRAY_USED_SLOT;
        buffer_destroy(item->key);

        switch (item->pool.flags & ~APE_POOL_ALL_FLAGS) {
            case APE_ARRAY_VAL_BUF:
                buffer_destroy(item->pool.ptr.buf);
                item->pool.flags &= ~APE_ARRAY_VAL_BUF;
                break;
            case APE_ARRAY_VAL_INT:
                break;
            default:
                break;
        }

        array->current = (ape_pool_t *)item;
    }
}

static ape_array_item_t *ape_array_add_s(ape_array_t *array, buffer *key)
{
    ape_array_item_t *slot;

    ape_array_delete(array, (const char *)key->data, key->used);

    slot = (ape_array_item_t *)array->current;

    if (slot == NULL || (slot->pool.flags & APE_ARRAY_USED_SLOT)) {
        slot = (ape_array_item_t *)ape_grow_pool(array, 4);
    }

    slot->pool.flags |= APE_ARRAY_USED_SLOT;

    slot->key = key;

    array->current = slot->pool.next;

    if (array->current == NULL
        || (((ape_array_item_t *)array->current)->pool.flags
            & APE_ARRAY_USED_SLOT)) {

        array->current = array->head;

        while (array->current != NULL
               && (((ape_array_item_t *)array->current)->pool.flags
                   & APE_ARRAY_USED_SLOT)) {
            array->current = ((ape_array_item_t *)array->current)->pool.next;
        }
    }

    return slot;
}

ape_array_item_t *ape_array_add_b(ape_array_t *array, buffer *key,
                                  buffer *value)
{
    ape_array_item_t *slot = ape_array_add_s(array, key);

    slot->pool.flags |= APE_ARRAY_VAL_BUF;
    slot->pool.ptr.buf = value;

    return slot;
}

ape_array_item_t *ape_array_add_ptr(ape_array_t *array, buffer *key, void *ptr)
{
    ape_array_item_t *slot = ape_array_add_s(array, key);

    slot->pool.ptr.data = ptr;

    return slot;
}

ape_array_item_t *ape_array_add_ptrn(ape_array_t *array, const char *key,
                                     int klen, void *ptr)
{
    buffer *k;
    k = buffer_new(klen + 1);

    buffer_append_string_n(k, key, klen);

    return ape_array_add_ptr(array, k, ptr);
}

ape_array_item_t *ape_array_add_n(ape_array_t *array, const char *key, int klen,
                                  const char *value, int vlen)
{
    buffer *k, *v;

    k = buffer_new(klen + 1);
    v = buffer_new(vlen + 1);

    buffer_append_string_n(k, key, klen);
    buffer_append_string_n(v, value, vlen);

    return ape_array_add_b(array, k, v);
}

ape_array_item_t *ape_array_add_camelkey_n(ape_array_t *array, const char *key,
                                           int klen, const char *value,
                                           int vlen)
{
    buffer *k, *v;

    k = buffer_new(klen + 1);
    v = buffer_new(vlen + 1);

    buffer_append_string_n(k, key, klen);
    buffer_append_string_n(v, value, vlen);

    buffer_camelify(k);

    return ape_array_add_b(array, k, v);
}


ape_array_item_t *ape_array_add(ape_array_t *array, const char *key,
                                const char *value)
{
    return ape_array_add_n(array, key, strlen(key), value, strlen(value));
}

void ape_array_destroy(ape_array_t *array)
{
    if (!array) {
        return;
    }
    ape_destroy_pool_list_ordered((ape_pool_list_t *)array, ape_array_clean_cb,
                                  NULL);
}

static void ape_array_clean_cb(ape_pool_t *item, void *ctx)
{
    ape_array_item_t *array = (ape_array_item_t *)item;

    if (!(array->pool.flags & APE_ARRAY_USED_SLOT)) {
        return;
    }
    array->pool.flags &= ~APE_ARRAY_USED_SLOT;

    buffer_destroy(array->key);

    switch (array->pool.flags & ~APE_POOL_ALL_FLAGS) {
        case APE_ARRAY_VAL_BUF:
            buffer_destroy(array->pool.ptr.buf);
            break;
        case APE_ARRAY_VAL_INT:
        default:
            break;
    }
}

