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

int ve_enclave_get_settings(ve_enclave_t* enclave, ve_enclave_settings_t* buf);

#endif /* _VE_HOST_ENCLAVE_H */
