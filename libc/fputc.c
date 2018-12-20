// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <stdio.h>
#include "stdio_impl.h"
#define fputc musl_fputc
#include "../3rdparty/musl/musl/src/stdio/fputc.c"
