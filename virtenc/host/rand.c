// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "rand.h"
#include <openenclave/internal/utils.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

static void _initialize_once(void)
{
    srand((uint32_t)time(NULL));
}

uint64_t ve_rand(void)
{
    static pthread_once_t _once = PTHREAD_ONCE_INIT;

    pthread_once(&_once, _initialize_once);

    uint64_t hi = (uint64_t)rand();
    uint64_t lo = (uint64_t)rand();
    return ((hi << 32) | lo);
}
