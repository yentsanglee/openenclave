// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stddef.h>
#include <stdlib.h>

void* ve_malloc(size_t size)
{
    return malloc(size);
}

void ve_free(void* ptr)
{
    return free(ptr);
}
