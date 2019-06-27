// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include "globals.h"
#include "print.h"

bool oe_is_within_enclave(const void* ptr, size_t size)
{
    const uint64_t min = (uint64_t)ptr;
    const uint64_t max = (uint64_t)ptr + size;
    const uint64_t lo = (uint64_t)globals.shmaddr;
    const uint64_t hi = (uint64_t)globals.shmaddr + globals.shmsize;

    if (min >= lo && min < hi)
        return false;

    if (max >= lo && max <= hi)
        return false;

    return true;
}

bool oe_is_outside_enclave(const void* ptr, size_t size)
{
    const uint64_t min = (uint64_t)ptr;
    const uint64_t max = (uint64_t)ptr + size;
    const uint64_t lo = (uint64_t)globals.shmaddr;
    const uint64_t hi = (uint64_t)globals.shmaddr + globals.shmsize;

    if (min < lo || min >= hi)
        return false;

    if (max < lo || max > hi)
        return false;

    return true;
}
