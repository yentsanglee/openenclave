// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_HOST_SWITCHLESS_H
#define _OE_HOST_SWITCHLESS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/sgxtypes.h>
#include <openenclave/internal/switchless.h>
#include "hostthread.h"

OE_EXTERNC_BEGIN

typedef struct _oe_switchless_state
{
    bool configured;
    bool initialized;
    uint32_t max_num_enclave_workers;
    uint32_t max_num_host_workers;
    uint32_t num_active_enclave_workers;
    uint32_t num_active_host_workers;
    uint32_t lock;
    oe_thread host_workers[OE_SGX_MAX_TCS];
    oe_thread enclave_workers[OE_SGX_MAX_TCS];
    oe_switchless_context_t ecall_contexts[OE_SGX_MAX_TCS];
    oe_switchless_context_t ocall_contexts[OE_SGX_MAX_TCS];
} oe_switchless_state_t;

oe_result_t oe_initialize_switchless(oe_enclave_t* enclave);
oe_result_t oe_terminate_switchless(oe_enclave_t* enclave);

OE_EXTERNC_END

#endif // _OE_HOST_SWITCHLESS_H
