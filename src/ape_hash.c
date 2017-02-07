/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include "port/windows.h"
#endif

#include "ape_hash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define HACH_TABLE_MAX 8209

extern unsigned long _ape_seed;

#if 0
uint64_t uniqid(const char *seed_key, int len)
{
    struct timeval tseed;

    gettimeofday(&tseed, NULL);

    //return hash(seed_key, len, (tseed.tv_sec & 0x00FFFFFF) | tseed.tv_usec);
}
#endif

uint32_t ape_hash_str(const void *key, int len, int max)
{
    return MurmurHash2(key, len, _ape_seed) % max;
}

ape_htable_t *hashtbl_init_with_size(ape_hash_type type, int table_size)
{
    ape_htable_item_t **htbl_item;
    ape_htable_t *htbl;

    htbl = malloc(sizeof(*htbl));

    htbl_item = (ape_htable_item_t **)malloc(sizeof(*htbl_item) * (table_size));

    memset(htbl_item, 0, sizeof(*htbl_item) * (table_size));

    htbl->size    = table_size;
    htbl->first   = NULL;
    htbl->table   = htbl_item;
    htbl->type    = type;
    htbl->cleaner = NULL;

    return htbl;
}

ape_htable_t *hashtbl_init(ape_hash_type type)
{
    return hashtbl_init_with_size(type, HACH_TABLE_MAX);
}

void hashtbl_set_cleaner(ape_htable_t *htbl, ape_hash_clean_callback cleaner)
{
    htbl->cleaner = cleaner;
}

void hashtbl_free(ape_htable_t *htbl)
{
    size_t i;
    ape_htable_item_t *hTmp;
    ape_htable_item_t *hNext;

    for (i = 0; i < htbl->size; i++) {
        hTmp = htbl->table[i];
        while (hTmp != 0) {
            hNext = hTmp->next;

            if (htbl->cleaner) {
                htbl->cleaner(hTmp);
            }

            if (htbl->type == APE_HASH_STR) {
                free(hTmp->key.str);
                hTmp->key.str = NULL;
            }
            free(hTmp);
            hTmp = hNext;
        }
    }

    free(htbl->table);
    free(htbl);
}

void hashtbl_append64(ape_htable_t *htbl, uint64_t key, void *structaddr)
{
    unsigned int key_hash;
    ape_htable_item_t *hTmp, *hDbl;

    key_hash = key % htbl->size;

    hTmp = (ape_htable_item_t *)malloc(sizeof(*hTmp));

    hTmp->next  = NULL;
    hTmp->lnext = htbl->first;
    hTmp->lprev = NULL;

    hTmp->key.integer = key;

    hTmp->content.addrs = (void *)structaddr;

    if (htbl->table[key_hash] != NULL) {
        hDbl = htbl->table[key_hash];

        while (hDbl != NULL) {
            if (key == hDbl->key.integer) {
                if (htbl->cleaner) {
                    htbl->cleaner(hTmp);
                }
                free(hTmp);
                hDbl->content.addrs = (void *)structaddr;
                return;
            } else {
                hDbl = hDbl->next;
            }
        }
        hTmp->next = htbl->table[key_hash];
    }

    if (htbl->first != NULL) {
        htbl->first->lprev = hTmp;
    }

    htbl->first = hTmp;

    htbl->table[key_hash] = hTmp;
}


void hashtbl_erase64(ape_htable_t *htbl, uint64_t key)
{
    unsigned int key_hash;
    ape_htable_item_t *hTmp, *hPrev;

    key_hash = key % htbl->size;

    hTmp  = htbl->table[key_hash];
    hPrev = NULL;

    while (hTmp != NULL) {
        if (key == hTmp->key.integer) {

            if (htbl->cleaner) {
                htbl->cleaner(hTmp);
            }

            if (hPrev != NULL) {
                hPrev->next = hTmp->next;
            } else {
                htbl->table[key_hash] = hTmp->next;
            }

            if (hTmp->lprev == NULL) {
                htbl->first = hTmp->lnext;
            } else {
                hTmp->lprev->lnext = hTmp->lnext;
            }
            if (hTmp->lnext != NULL) {
                hTmp->lnext->lprev = hTmp->lprev;
            }

            hTmp->key.integer = 0;

            free(hTmp);
            return;
        }
        hPrev = hTmp;
        hTmp  = hTmp->next;
    }
}

void *hashtbl_seek64(ape_htable_t *htbl, uint64_t key)
{
    unsigned int key_hash;
    ape_htable_item_t *hTmp;

    key_hash = key % htbl->size;

    hTmp = htbl->table[key_hash];

    while (hTmp != NULL) {
        if (key == hTmp->key.integer) {
            return (void *)(hTmp->content.addrs);
        }
        hTmp = hTmp->next;
    }

    return NULL;
}

void hashtbl_append_val32(ape_htable_t *htbl, const char *key, int key_len,
                          uint32_t val)
{
    unsigned int key_hash;
    ape_htable_item_t *hTmp, *hDbl;

    if (key == NULL) {
        return;
    }

    key_hash = ape_hash_str(key, key_len, htbl->size);

    hTmp = (ape_htable_item_t *)malloc(sizeof(*hTmp));

    hTmp->next  = NULL;
    hTmp->lnext = htbl->first;
    hTmp->lprev = NULL;

    hTmp->key.str = malloc(sizeof(char) * (key_len + 1));

    hTmp->content.scalar = val;

    memcpy(hTmp->key.str, key, key_len + 1);

    if (htbl->table[key_hash] != NULL) {
        hDbl = htbl->table[key_hash];

        while (hDbl != NULL) {
            if (strcasecmp(hDbl->key.str, key) == 0) {
                free(hTmp->key.str);
                free(hTmp);
                hDbl->content.scalar = val;
                return;
            } else {
                hDbl = hDbl->next;
            }
        }
        hTmp->next = htbl->table[key_hash];
    }

    if (htbl->first != NULL) {
        htbl->first->lprev = hTmp;
    }

    htbl->first = hTmp;

    htbl->table[key_hash] = hTmp;
}

void hashtbl_append(ape_htable_t *htbl, const char *key, int key_len,
                    void *structaddr)
{
    unsigned int key_hash;
    ape_htable_item_t *hTmp, *hDbl;

    if (key == NULL) {
        return;
    }

    key_hash = ape_hash_str(key, key_len, htbl->size);

    hTmp = (ape_htable_item_t *)malloc(sizeof(*hTmp));

    hTmp->next  = NULL;
    hTmp->lnext = htbl->first;
    hTmp->lprev = NULL;

    hTmp->key.str = malloc(sizeof(char) * (key_len + 1));

    hTmp->content.addrs = (void *)structaddr;

    memcpy(hTmp->key.str, key, key_len + 1);

    if (htbl->table[key_hash] != NULL) {
        hDbl = htbl->table[key_hash];

        while (hDbl != NULL) {
            if (strcasecmp(hDbl->key.str, key) == 0) {
                if (htbl->cleaner) {
                    htbl->cleaner(hTmp);
                }
                free(hTmp->key.str);
                free(hTmp);
                hDbl->content.addrs = (void *)structaddr;
                return;
            } else {
                hDbl = hDbl->next;
            }
        }
        hTmp->next = htbl->table[key_hash];
    }

    if (htbl->first != NULL) {
        htbl->first->lprev = hTmp;
    }

    htbl->first = hTmp;

    htbl->table[key_hash] = hTmp;
}


void hashtbl_erase(ape_htable_t *htbl, const char *key, int key_len)
{
    unsigned int key_hash;
    ape_htable_item_t *hTmp, *hPrev;

    if (key == NULL) {
        return;
    }

    key_hash = ape_hash_str(key, key_len, htbl->size);

    hTmp  = htbl->table[key_hash];
    hPrev = NULL;

    while (hTmp != NULL) {
        if (strcasecmp(hTmp->key.str, key) == 0) {
            if (htbl->cleaner) {
                htbl->cleaner(hTmp);
            }
            if (hPrev != NULL) {
                hPrev->next = hTmp->next;
            } else {
                htbl->table[key_hash] = hTmp->next;
            }

            if (hTmp->lprev == NULL) {
                htbl->first = hTmp->lnext;
            } else {
                hTmp->lprev->lnext = hTmp->lnext;
            }
            if (hTmp->lnext != NULL) {
                hTmp->lnext->lprev = hTmp->lprev;
            }

            free(hTmp->key.str);
            free(hTmp);
            return;
        }
        hPrev = hTmp;
        hTmp  = hTmp->next;
    }
}

void *hashtbl_seek(ape_htable_t *htbl, const char *key, int key_len)
{
    unsigned int key_hash;
    ape_htable_item_t *hTmp;

    if (key == NULL) {
        return NULL;
    }

    key_hash = ape_hash_str(key, key_len, htbl->size);

    hTmp = htbl->table[key_hash];

    while (hTmp != NULL) {
        if (strcasecmp(hTmp->key.str, key) == 0) {
            return (void *)(hTmp->content.addrs);
        }
        hTmp = hTmp->next;
    }

    return NULL;
}

uint32_t hashtbl_seek_val32(ape_htable_t *htbl, const char *key, int key_len)
{
    unsigned int key_hash;
    ape_htable_item_t *hTmp;

    if (key == NULL) {
        return 0;
    }

    key_hash = ape_hash_str(key, key_len, htbl->size);

    hTmp = htbl->table[key_hash];

    while (hTmp != NULL) {
        if (strcasecmp(hTmp->key.str, key) == 0) {
            return hTmp->content.scalar;
        }
        hTmp = hTmp->next;
    }

    return 0;
}

//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby

unsigned int MurmurHash2(const void *key, int len, unsigned int seed)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const unsigned int m = 0x5bd1e995;
    const int r          = 24;

    // Initialize the hash to a 'random' value

    unsigned int h = seed ^ len;

    // Mix 4 bytes at a time into the hash

    const unsigned char *data = (const unsigned char *)key;

    while (len >= 4) {
        unsigned int k = *(unsigned int *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch (len) {
        case 3:
            h ^= data[2] << 16;
        case 2:
            h ^= data[1] << 8;
        case 1:
            h ^= data[0];
            h *= m;
            break;
        default:
            break;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

