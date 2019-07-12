// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_ENCLAVE_H
#define _VE_HOST_ENCLAVE_H

#include <openenclave/bits/defs.h>
#include <openenclave/host.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include "calls.h"
#include "heap.h"

#define MAX_THREADS 1024

typedef struct _ve_enclave ve_enclave_t;

int ve_enclave_create(
    const char* path,
    ve_heap_t* heap,
    ve_enclave_t** enclave_out);

int ve_enclave_terminate(ve_enclave_t* enclave);

void ve_enclave_set_data(ve_enclave_t* enclave, void* data);

void* ve_enclave_get_data(ve_enclave_t* enclave);

ve_enclave_t* ve_enclave_cast(void* ptr);

int ve_enclave_get_settings(ve_enclave_t* enclave, ve_enclave_settings_t* buf);

/* Assign thread and call into the enclave. */
oe_result_t ve_enclave_call(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6);

OE_INLINE oe_result_t
ve_enclave_call0(ve_enclave_t* enclave, ve_func_t func, uint64_t* retval)
{
    return ve_enclave_call(enclave, func, retval, 0, 0, 0, 0, 0, 0);
}

OE_INLINE oe_result_t ve_enclave_call1(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1)
{
    return ve_enclave_call(enclave, func, retval, arg1, 0, 0, 0, 0, 0);
}

OE_INLINE oe_result_t ve_enclave_call2(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2)
{
    return ve_enclave_call(enclave, func, retval, arg1, arg2, 0, 0, 0, 0);
}

OE_INLINE oe_result_t ve_enclave_call3(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    return ve_enclave_call(enclave, func, retval, arg1, arg2, arg3, 0, 0, 0);
}

OE_INLINE oe_result_t ve_enclave_call4(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4)
{
    return ve_enclave_call(enclave, func, retval, arg1, arg2, arg3, arg4, 0, 0);
}

OE_INLINE oe_result_t ve_enclave_call5(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    return ve_enclave_call(
        enclave, func, retval, arg1, arg2, arg3, arg4, arg5, 0);
}

OE_INLINE oe_result_t ve_enclave_call6(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6)
{
    return ve_enclave_call(
        enclave, func, retval, arg1, arg2, arg3, arg4, arg5, arg6);
}

#endif /* _VE_HOST_ENCLAVE_H */
