// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/print.h>
#include "vsample1_t.h"

uint64_t test_ecall(uint64_t x)
{
    uint64_t retval;

#if 0
    oe_malloc(100);
#endif

    if (test_ocall(&retval, x) != OE_OK)
    {
        oe_host_printf("test_ocall() failed\n");
        oe_abort();
    }

    oe_host_printf("retval=%lu\n", retval);

    return retval;
}

int test1(char* in, char out[100])
{
    if (in)
        oe_strlcpy(out, in, 100);

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    256,  /* StackPageCount */
    8);   /* TCSCount */
