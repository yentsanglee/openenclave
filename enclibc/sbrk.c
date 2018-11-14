// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <unistd.h>
#include <openenclave/internal/enclavelibc.h>

void* enclibc_sbrk(ptrdiff_t increment)
{
    return oe_sbrk(increment);
}
