// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include "print.h"
#include "process.h"
#include "string.h"
#include "test_t.h"

uint64_t test_ecall(uint64_t x)
{
    uint64_t retval;

    if (test_ocall(&retval, x) != OE_OK)
        ve_panic("test_ocall() failed");

    ve_print("retval=%lu\n", retval);

    return retval;
}

int test1(char* in, char out[100])
{
    if (in)
        ve_strlcpy(out, in, 100);

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    256,  /* StackPageCount */
    8);   /* TCSCount */
