// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdlib.h>
#include <string.h>

static void* _zero_filling_malloc(size_t size)
{
    void* ptr;

    if ((ptr = malloc(size)))
        memset(ptr, 0, size);

    return ptr;
}

/* Use a zero-filling malloc to workaround Valgrind errors. */
#define malloc _zero_filling_malloc

#include "../3rdparty/musl/musl/src/stdio/__fdopen.c"
