// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include "test_u.h"

uint64_t test_ocall(uint64_t x)
{
    printf("*** test_ocall()\n");
    return x;
}
