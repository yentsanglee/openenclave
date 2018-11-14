// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/enclavelibc.h>

int enclibc_atexit(void (*function)(void))
{
    return oe_atexit(function);
}
