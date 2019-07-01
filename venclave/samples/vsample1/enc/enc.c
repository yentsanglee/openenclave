// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/thread.h>
#include "vsample1_t.h"

uint64_t test_ecall(uint64_t x)
{
    uint64_t retval;

    static oe_mutex_t _mutex = OE_MUTEX_INITIALIZER;
    oe_mutex_lock(&_mutex);

    if (test_ocall(&retval, x) != OE_OK)
    {
        oe_host_printf("test_ocall() failed\n");
        oe_abort();
    }

    oe_mutex_unlock(&_mutex);

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
