// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_THREAD_H
#define _VE_ENCLAVE_THREAD_H

#include "common.h"
#include "lock.h"

#define VE_MAGIC_TLS_INITIALIZER 0xad3c6c3491a64801

typedef struct _ve_thread* ve_thread_t;

ve_thread_t ve_thread_self(void);

int ve_thread_create(
    ve_thread_t* thread,
    int (*func)(void*),
    void* arg,
    size_t stack_size);

int ve_thread_join(ve_thread_t thread, int* retval);

int ve_thread_set_retval(int retval);

volatile int* ve_thread_get_futex_addr(ve_thread_t thread);

/* Return pointer to the two extra pages that follow the thread struct. */
uint8_t* ve_thread_get_extra(ve_thread_t thread);

/* Reinitialize the thread's TLS to its original values. */
int ve_thread_reinitialize_tls(void);

/* Set the socket for the current thread. */
void ve_thread_set_sock(int sock);

/* Get the socket for the current thread. */
int ve_thread_get_sock(void);

#endif /* _VE_ENCLAVE_THREAD_H */
