// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_ENCLAVE_H
#define _VE_HOST_ENCLAVE_H

#include <stddef.h>
#include <stdint.h>
#include "call.h"
#include "heap.h"

typedef struct _ve_enclave ve_enclave_t;

int ve_enclave_create(
    const char* path,
    ve_heap_t* heap,
    ve_enclave_t** enclave_out);

int ve_enclave_terminate(ve_enclave_t* enclave);

int ve_enclave_ping(ve_enclave_t* enclave, uint64_t tcs, uint64_t ping_value);

int ve_enclave_call(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6);

int ve_enclave_get_settings(ve_enclave_t* enclave, ve_enclave_settings_t* buf);

int ve_enclave_run_xor_test(ve_enclave_t* enclave);

#endif /* _VE_HOST_ENCLAVE_H */
