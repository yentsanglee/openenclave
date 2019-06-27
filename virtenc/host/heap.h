// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_HEAP_H
#define _VE_HOST_HEAP_H

#include <stddef.h>

#define VE_HEAP_SIZE (1024 * 1024)

/* The malloc heap shared with enclaves. */
typedef struct _ve_heap
{
    int shmid;
    void* shmaddr;
    size_t shmsize;
} ve_heap_t;

int ve_heap_create(ve_heap_t* heap, size_t heap_size);

#endif /* _VE_HOST_HEAP_H */
