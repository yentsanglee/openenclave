// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/error.h>
#include <openenclave/internal/sgxtypes.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "../../../../enclave/core/sgx/td.h"

// The debugger no loger relies on any OE specific structures on enlave side.
// The stack stitching for ocalls is done by the SDK itself.
// Note: The debugger uses SGX defined sgx_tcs_t for setting the debug-optin
// flag.
void assert_debugger_binary_contract_enclave_side()
{
    printf("Debugger contract validated on enclave side.\n");
}

void enc_assert_debugger_binary_contract()
{
    assert_debugger_binary_contract_enclave_side();
}
