// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdlib.h>
#include <openenclave/internal/enclavelibc.h>

void* enclibc_malloc(size_t size)
{
    return oe_malloc(size);
}

void enclibc_free(void* ptr)
{
    oe_free(ptr);
}

void* enclibc_calloc(size_t nmemb, size_t size)
{
    return oe_calloc(nmemb, size);
}

void* enclibc_realloc(void* ptr, size_t size)
{
    return oe_realloc(ptr, size);
}

int enclibc_posix_memalign(void** memptr, size_t alignment, size_t size)
{
    return oe_posix_memalign(memptr, alignment, size);
}

void* enclibc_memalign(size_t alignment, size_t size)
{
    return oe_memalign(alignment, size);
}
