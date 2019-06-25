// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_THREAD_H
#define _VE_ENCLAVE_THREAD_H

#include <openenclave/bits/types.h>
#include <openenclave/internal/defs.h>
#include "lock.h"

#define VE_MAX_THREADS 1024
#define VE_PAGE_SIZE 4096
#define VE_TLS_SIZE (1 * VE_PAGE_SIZE)

/* The thread ABI for Linux x86-64: do not change! */
typedef struct _thread_base
{
    struct _thread_base* self;
    uint64_t dtv;
    uint64_t unused1;
    uint64_t unused2;
    uint64_t sysinfo;
    uint64_t canary;
    uint64_t canary2;
} thread_base_t;

/* Represents a thread (the FS register points to instances of this) */
typedef struct _thread
{
    thread_base_t base;

    /* Internal implementation. */
    struct _thread* next;
    void* tls;
    size_t tls_size;
    void* stack;
    size_t stack_size;
    uint64_t tcs;
    int sock;
    int ptid;
    int ctid;
    int unused;
    uint8_t padding[3972];
} thread_t;

OE_STATIC_ASSERT(sizeof(thread_t) == VE_PAGE_SIZE);

void ve_dump_thread(thread_t* thread);

thread_t* ve_thread_self(void);

#endif /* _VE_ENCLAVE_THREAD_H */
