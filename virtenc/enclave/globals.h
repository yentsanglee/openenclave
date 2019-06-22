// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_GLOBALS_H
#define _VE_ENCLAVE_GLOBALS_H

#include <openenclave/bits/types.h>
#include "lock.h"

#define MAX_THREADS 1024

typedef struct _thread
{
    int sock;
    void* stack;
    size_t stack_size;
    uint32_t tcs;
    int tid;
} thread_t;

typedef struct _threads
{
    thread_t data[MAX_THREADS];
    size_t size;
    ve_lock_t lock;
} threads_t;

typedef struct _globals
{
    int sock;
    threads_t threads;

    /* Shared memory between host and enclave. */
    void* shmaddr;
} globals_t;

extern globals_t globals;

#endif /* _VE_ENCLAVE_GLOBALS_H */
