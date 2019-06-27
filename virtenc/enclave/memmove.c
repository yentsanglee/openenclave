// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/types.h>

#pragma GCC diagnostic ignored "-Wsign-conversion"

void* memcpy(void* dest, const void* src, size_t n);

#include "../../3rdparty/musl/musl/src/string/memmove.c"
