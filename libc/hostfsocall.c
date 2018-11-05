// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/raise.h>
#include "fs/hostfs.h"

oe_result_t oe_hostfs_ocall(fs_hostfs_ocall_args_t* args)
{
    oe_result_t result = OE_OK;

    if (!args)
        OE_RAISE(OE_UNEXPECTED);

    OE_CHECK(oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL));

    result = OE_OK;

done:
    return result;
}
