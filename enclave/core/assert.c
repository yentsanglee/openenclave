// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/backtrace.h>
#include <openenclave/internal/types.h>

void __oe_assert_fail(
    const char* expr,
    const char* file,
    int line,
    const char* function)
{
    oe_host_printf(
        "Assertion failed: %s (%s: %s: %d)\n", expr, file, function, line);
#ifdef OE_USE_DEBUG_MALLOC
    oe_print_backtrace();
#endif /* OE_USE_DEBUG_MALLOC */
    oe_abort();
}
