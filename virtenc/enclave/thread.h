// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_THREAD_H
#define _VE_ENCLAVE_THREAD_H

#include "common.h"
#include "lock.h"

typedef struct _ve_thread* ve_thread_t;

ve_thread_t ve_thread_self(void);

int ve_thread_create(
    ve_thread_t* thread,
    int (*func)(void*),
    void* arg,
    size_t stack_size);

int ve_thread_join(ve_thread_t thread, int* retval);

int ve_thread_tid(ve_thread_t thread);

#endif /* _VE_ENCLAVE_THREAD_H */
