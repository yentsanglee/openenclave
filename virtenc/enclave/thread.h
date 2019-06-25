// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_THREAD_H
#define _VE_ENCLAVE_THREAD_H

#include <openenclave/bits/types.h>
#include <openenclave/internal/defs.h>
#include "lock.h"

typedef struct _ve_thread
{
    void* __impl;
} ve_thread_t;

int ve_thread_create(
    ve_thread_t* thread,
    int (*func)(void*),
    void* arg,
    size_t stack_size);

ve_thread_t ve_thread_self(void);

int ve_thread_set_destructor(void (*destructor)(void*), void* arg);

int ve_thread_join_all(void);

#endif /* _VE_ENCLAVE_THREAD_H */
