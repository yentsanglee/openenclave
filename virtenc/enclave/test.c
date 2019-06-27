// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include "print.h"
#include "process.h"
#include "string.h"
#include "test_t.h"

int test0(void)
{
    uint64_t retval;

    if (test_ocall(&retval, 12345) != OE_OK)
        ve_panic("test_ocall() failed");

    ve_print("retval=%lu\n", retval);

    return (int)retval;
}

int test1(char* in, char out[100])
{
    if (in)
        ve_strlcpy(out, in, 100);

    return 0;
}
