// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../edltestutils.h"

#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <algorithm>
#include "all_t.h"

int add_switchless(int a, int b)
{
    return a + b;
}

int add_normal(int a, int b)
{
    return a + b;
}
