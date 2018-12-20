// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <stdio.h>
#include "stdio_impl.h"
#undef weak_alias
#define weak_alias(...)
#define getc musl_getc
#include "../3rdparty/musl/musl/src/stdio/getc.c"
#include "weak_alias.h"

weak_alias(musl_getc, _IO_getc);
