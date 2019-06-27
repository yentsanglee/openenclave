// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/raise.h>
#include "print.h"

oe_result_t oe_log(log_level_t level, const char* fmt, ...)
{
    oe_va_list ap;

    oe_va_start(ap, fmt);
    ve_print("oe_log: %u: ", level);
    ve_vprint(fmt, ap);
    oe_va_end(ap);

    return OE_OK;
}
