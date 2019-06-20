// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_GLOBALS_H
#define _VE_HOST_GLOBALS_H

#include <stddef.h>
#include "../common/lock.h"

#define MAX_THREADS 1024

typedef struct _thread_info
{
    int sock;
    int child_sock;
    uint32_t tcs;
} thread_info_t;

typedef struct _globals
{
    int sock;
    int child_sock;
    thread_info_t threads[MAX_THREADS];
    ve_lock_t threads_lock;
    size_t num_threads;
} globals_t;

extern globals_t globals;

#endif /* _VE_HOST_GLOBALS_H */
