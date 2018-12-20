// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include "stdio_impl.h"

#define vfprintf musl_vfprintf
#include "../3rdparty/musl/musl/src/stdio/vfprintf.c"
