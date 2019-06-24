// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "assert.h"
#include "print.h"
#include "process.h"

void __ve_assert_fail(
    const char* expr,
    const char* file,
    int line,
    const char* function)
{
    ve_print("Assertion failed: %s (%s: %s: %d)\n", expr, file, function, line);
    ve_abort();
}
