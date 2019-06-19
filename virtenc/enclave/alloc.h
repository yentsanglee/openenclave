// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ALLOC_H
#define _VE_ALLOC_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

/* Allocate permanent memory from the heap with the default alignment. */
void* ve_alloc(size_t size);

/* Allocate permanent memory form the heap with the given alignment. */
void* ve_alloc_aligned(size_t size, size_t alignment);

#endif /* _VE_ALLOC_H */
