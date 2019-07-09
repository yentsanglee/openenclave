// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "panic.h"
#include "print.h"
#include "process.h"

void __ve_panic(
    const char* expr,
    const char* file,
    int line,
    const char* function)
{
    ve_printf("Panicked: %s (%s: %s: %d)\n", expr, file, function, line);
    ve_abort();
}
