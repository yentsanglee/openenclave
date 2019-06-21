// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <stdlib.h>
#include <string.h>
#include "../common/lock.h"
#include "globals.h"
#include "malloc.h"

#if defined(M_TRIM_THRESHOLD)
#undef M_TRIM_THRESHOLD
#endif

#if defined(M_MMAP_THRESHOLD)
#undef M_MMAP_THRESHOLD
#endif

static void* sbrk(intptr_t increment)
{
    void* ret = (void*)-1;
    static uint8_t* _brk_value;
    static ve_lock_t _lock;

    ve_lock(&_lock);

    /* Initialize the first time. */
    if (_brk_value == NULL)
        _brk_value = (uint8_t*)globals.shmaddr;

    /* If increment is zero, then return the current break value. */
    if (increment == 0)
    {
        ret = _brk_value;
        goto done;
    }

    /* Increment the break value and return the old break value. */
    {
        uint8_t* old_brk_value = _brk_value;
        uint8_t* new_brk_value = _brk_value + increment;
        uint8_t* start = (uint8_t*)globals.shmaddr;
        uint8_t* end = start + globals.shmsize;

        /* If the shared memory heap is exhausted. */
        if (!(new_brk_value >= start && new_brk_value <= end))
        {
            ret = (void*)-1;
            goto done;
        }

        _brk_value = new_brk_value;
        ret = old_brk_value;
        goto done;
    }

done:

    ve_unlock(&_lock);

    return ret;
}

static const int EINVAL = 22;
static const int ENOMEM = 12;

#define HAVE_MMAP 0
#define LACKS_UNISTD_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_TYPES_H
#define LACKS_TIME_H
#define LACKS_STDLIB_H
#define LACKS_STRING_H
#define LACKS_ERRNO_H 1
#define LACKS_SCHED_H 1
#define NO_MALLOC_STATS 1
#define MALLOC_FAILURE_ACTION
#define USE_LOCKS 1
#define USE_DL_PREFIX
#define STRUCT_MALLINFO_DECLARED

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wparentheses-equality"
#endif
#include "../../3rdparty/dlmalloc/dlmalloc/malloc.c"
#pragma GCC diagnostic pop

void* ve_host_malloc(size_t size)
{
    return dlmalloc(size);
}

void ve_host_free(void* ptr)
{
    return dlfree(ptr);
}

void* ve_host_calloc(size_t nmemb, size_t size)
{
    return dlcalloc(nmemb, size);
}

void* ve_host_realloc(void* ptr, size_t size)
{
    return dlrealloc(ptr, size);
}

void* ve_host_memalign(size_t alignment, size_t size)
{
    return dlmemalign(alignment, size);
}
