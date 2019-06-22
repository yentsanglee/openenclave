// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_GLOBALS_H
#define _VE_HOST_GLOBALS_H

#include <pthread.h>
#include <stddef.h>

#define MAX_THREADS 1024

#define HOST_HEAP_SIZE (1024 * 1024)

typedef struct _thread
{
    int sock;
    int child_sock;
    uint32_t tcs;
} thread_t;

typedef struct _threads
{
    thread_t data[MAX_THREADS];
    size_t size;
    pthread_spinlock_t lock;
} threads_t;

/* Host heap memory shared with child processes. */
typedef struct _heap
{
    int shmid;
    void* shmaddr;
    size_t shmsize;
} heap_t;

typedef struct _globals
{
    int sock;
    int child_sock;
    threads_t threads;
    heap_t heap;
} globals_t;

extern globals_t globals;

#endif /* _VE_HOST_GLOBALS_H */
