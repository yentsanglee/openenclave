// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "err.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

extern const char* __ve_arg0;

OE_PRINTF_FORMAT(1, 2)
void err(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", __ve_arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}
