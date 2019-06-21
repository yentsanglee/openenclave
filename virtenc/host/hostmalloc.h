// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOSTMALLOC_H
#define _VE_HOSTMALLOC_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

void* ve_host_malloc(size_t size);

void ve_host_free(void* ptr);

void* ve_host_calloc(size_t nmemb, size_t size);

void* ve_host_realloc(void* ptr, size_t size);

void* ve_host_memalign(size_t alignment, size_t size);

#endif /* _VE_HOSTMALLOC_H */
