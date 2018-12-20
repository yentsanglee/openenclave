// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <stdio.h>
#include "stdio_impl.h"
#define fclose musl_fclose
#include "../3rdparty/musl/musl/src/stdio/fclose.c"
