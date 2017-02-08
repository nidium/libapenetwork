/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#ifndef _APE_HASH_H
#define _APE_HASH_H

#include <stdint.h>


typedef struct _ape_htable_item ape_htable_item_t;

typedef void (*ape_hash_clean_callback)(ape_htable_item_t *);

typedef enum { APE_HASH_STR, APE_HASH_INT } ape_hash_type;

typedef struct _ape_htable {
    struct _ape_htable_item *first;
    struct _ape_htable_item **table;

    ape_hash_type type;
    ape_hash_clean_callback cleaner;

    unsigned size;

} ape_htable_t;


struct _ape_htable_item {
    union {
        char *str;
        uint64_t integer;
    } key;

    union {
        void *addrs;
        uint32_t scalar;
    } content;

    struct _ape_htable_item *next;

    struct _ape_htable_item *lnext;
    struct _ape_htable_item *lprev;
};

#ifdef __cplusplus
extern "C" {
#endif

ape_htable_t *hashtbl_init_with_size(ape_hash_type type, int table_size);
ape_htable_t *hashtbl_init(ape_hash_type type);

void hashtbl_set_cleaner(ape_htable_t *htbl, ape_hash_clean_callback cleaner);
void hashtbl_free(ape_htable_t *htbl);
void *hashtbl_seek64(ape_htable_t *htbl, uint64_t key);
void hashtbl_erase64(ape_htable_t *htbl, uint64_t key);
void hashtbl_append64(ape_htable_t *htbl, uint64_t key, void *structaddr);

uint32_t ape_hash_str(const void *key, int len, int max_size);
unsigned int MurmurHash2(const void *key, int len, unsigned int seed);
void hashtbl_append(ape_htable_t *htbl, const char *key, int key_len,
                    void *structaddr);

void hashtbl_append_val32(ape_htable_t *htbl, const char *key, int key_len,
                          uint32_t val);

void hashtbl_erase(ape_htable_t *htbl, const char *key, int key_len);
void *hashtbl_seek(ape_htable_t *htbl, const char *key, int key_len);
uint32_t hashtbl_seek_val32(ape_htable_t *htbl, const char *key, int key_len);

#ifdef __cplusplus
}
#endif

#endif

