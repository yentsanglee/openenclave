// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/mount.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include "ext2_t.h"

int test_ext2(const char* ext2_filename)
{
    oe_result_t r;

    r = oe_mount(OE_MOUNT_TYPE_EXT2, ext2_filename, ext2_filename);
    OE_TEST(r == OE_OK);

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
