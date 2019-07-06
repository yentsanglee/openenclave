// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <stdio.h>
#include "proxy_t.h"

void dummy_ecall(void)
{
    printf("oevproxyenc: dummy_ecall()\n");
    fflush(stdout);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    8);   /* TCSCount */
