// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include "print.h"
#include "string.h"
#include "test_t.h"

int test0(void)
{
    return 12345;
}

int test1(char* in, char out[100])
{
    if (in)
        ve_strlcpy(out, in, 100);

    return 0;
}
