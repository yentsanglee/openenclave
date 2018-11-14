// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdlib.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/atexit.h>

int enclibc_atexit(void (*function)(void))
{
    return oe_atexit(function);
}
