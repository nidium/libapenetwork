/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <ape_pool.h>

#include <stdio.h>

#define SIZE_STEP 16
#define SIZE_LIMIT 256
#define N_STEP 2
#define N_LIMIT 8 //512

#define REPEAT 2048

unsigned long _ape_seed = 31415961;

int main(const int argc, const char **argv)
{
    size_t repeat, size, n;
    for(repeat = 0; repeat < REPEAT; repeat++) {
        for(size = 0; size < SIZE_LIMIT; size += SIZE_STEP) {
            for(n = N_STEP; n < N_LIMIT; n += N_STEP) {
                printf("%d %d %d\n", (int)size, (int)n, (int)repeat);
                ape_pool_t * p = ape_new_pool(size, n);
                ape_destroy_pool(p);
            }
        }
    }

    return 0;
}

