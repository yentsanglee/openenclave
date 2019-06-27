// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_MALLOC_H
#define _VE_MALLOC_H

#include "common.h"

void* ve_malloc(size_t size);

void ve_free(void* ptr);

void* ve_calloc(size_t nmemb, size_t size);

void* ve_realloc(void* ptr, size_t size);

void* ve_memalign(size_t alignment, size_t size);

#endif /* _VE_MALLOC_H */
