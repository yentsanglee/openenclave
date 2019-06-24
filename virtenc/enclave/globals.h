// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_GLOBALS_H
#define _VE_ENCLAVE_GLOBALS_H

#include <openenclave/bits/types.h>
#include "thread.h"

typedef struct _globals
{
    thread_t* threads;
    ve_lock_t lock;

    /* Shared memory between host and enclave. */
    void* shmaddr;
} globals_t;

extern globals_t globals;

/* Socket used to communicate with the parent host process. */
extern int g_sock;

#endif /* _VE_ENCLAVE_GLOBALS_H */
