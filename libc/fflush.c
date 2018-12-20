// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <stdio.h>
int fflush(FILE *stream);
#include "stdio_impl.h"
#define fflush musl_fflush

static FILE *volatile _dummy = 0;
weak_alias(_dummy, __stdout_used);

#undef weak_alias
#define weak_alias(...)
#include "../3rdparty/musl/musl/src/stdio/fflush.c"

weak_alias(musl_fflush, fflush_unlocked);
