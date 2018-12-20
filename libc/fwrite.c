// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <stdio.h>
#include "stdio_impl.h"

#define fwrite musl_fwrite
#undef weak_alias
#define weak_alias(...)
#include "../3rdparty/musl/musl/src/stdio/fwrite.c"
#include "weak_alias.h"

weak_alias(musl_fwrite, fwrite_unlocked);
