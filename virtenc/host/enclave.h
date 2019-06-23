// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_ENCLAVE_H
#define _VE_HOST_ENCLAVE_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <pthread.h>

#define MAX_THREADS 1024

typedef struct _thread
{
    int sock;
    int child_sock;
    uint32_t tcs;
} thread_t;

typedef struct _ve_enclave
{
    int pid;
    int sock;
    int child_sock;

    thread_t threads[MAX_THREADS];
    size_t threads_size;
    pthread_spinlock_t threads_lock;
} ve_enclave_t;

int ve_create_enclave(const char* path, ve_enclave_t** enclave_out);

int ve_terminate_enclave(ve_enclave_t* enclave);

#endif /* _VE_HOST_ENCLAVE_H */
