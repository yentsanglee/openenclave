// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include "../fs/hostcalls.h"

static void* _malloc(fs_host_calls_t* host_calls, size_t size)
{
    return oe_host_malloc(size);
}

static void* _calloc(fs_host_calls_t* host_calls, size_t nmemb, size_t size)
{
    return oe_host_calloc(nmemb, size);
}

static void _free(fs_host_calls_t* host_calls, void* ptr)
{
    return oe_host_free(ptr);
}

static int _call(
    fs_host_calls_t* host_calls,
    const fs_guid_t* guid,
    fs_args_t* args)
{

    if (oe_ocall(OE_OCALL_FS, (uint64_t)args, NULL) != OE_OK)
        return -1;

    return 0;
}

fs_host_calls_t fs_host_calls = {_malloc, _calloc, _free, _call};
