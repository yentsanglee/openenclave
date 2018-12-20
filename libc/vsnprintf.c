// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>

int musl_vfprintf(FILE* stream, const char* format, va_list ap);

#define vfprintf musl_vfprintf
#include "../3rdparty/musl/musl/src/stdio/vsnprintf.c"
