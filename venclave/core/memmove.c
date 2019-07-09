// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "common.h"

#pragma GCC diagnostic ignored "-Wsign-conversion"

void* memcpy(void* dest, const void* src, size_t n);

#define memmove __musl_memmove

#include "../../3rdparty/musl/musl/src/string/memmove.c"

#undef memmove

/* Define this as weak since MUSL also defines one. */
__attribute__((__weak__)) void* memmove(void* dest, const void* src, size_t n)
{
    return __musl_memmove(dest, src, n);
}
