/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

#include <stdio.h>

#include <ape_netlib.h>
#include <ape_dns.h>


ape_global * g_ape;

#define MIN_TIMEOUT 8

static int resolve_cb(const char *ip, void * arg, int status)
{
    APE_loop_stop();
    return 1;
}


static int interval_cb(void *param) {
    int *p;

    p = (int*) param;
    return *p;
}

int main(const int argc, const char **argv)
{
    int minTime, resolve;

    g_ape = APE_init();

    resolve = 0;
    if (argc == 1) {
        minTime = MIN_TIMEOUT;
    } else if (argc == 2) {
        minTime = atoi(argv[1]);
    } else if (argc == 3) {
        minTime = atoi(argv[1]);
        resolve = 1;
    } else {
        printf("Usage: %s [timeout] [resolve]\n\tdefault timeout: %d\n\tactivate dns socket: %d", argv[0], MIN_TIMEOUT, resolve);
        exit(1);
    }
    printf("starting interval with %d with%s resolving\n", minTime, (resolve)?"":"out");
    ape_gethostbyname("nidium.com", resolve_cb, NULL, g_ape);
    if (resolve){
        APE_timer_create(g_ape, minTime, interval_cb, &minTime);
    }

    APE_loop_run(g_ape);
    APE_destroy(g_ape);

    return 0;
}

