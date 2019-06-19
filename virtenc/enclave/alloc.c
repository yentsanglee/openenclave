// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "alloc.h"
#include <openenclave/bits/types.h>
#include "lock.h"
#include "sbrk.h"

#define DEFAULT_ALIGNMENT 16

static uint64_t _round_up_to_multiple(uint64_t x, uint64_t m)
{
    return (x + m - 1) / m * m;
}

void* ve_alloc_aligned(size_t size, size_t alignment)
{
    void* ret = NULL;
    uint64_t break_value;
    uint64_t rounded_break_value;
    uint8_t* ptr;
    bool locked = false;
    static ve_lock_t _lock;

    /* Fail if size is too big to be cast to intptr_t. */
    if ((intptr_t)size < 0)
        goto done;

    /* Fail if alignment is unreasonably large. */
    if ((intptr_t)alignment < 0)
        goto done;

    ve_lock(&_lock);
    locked = true;

    /* Adjust break value to the alignment boundary. */
    if (alignment > 1)
    {
        /* Get the current break value. */
        if ((break_value = (uint64_t)ve_sbrk(0)) == (uint64_t)-1)
            goto done;

        rounded_break_value = _round_up_to_multiple(break_value, alignment);

        if (break_value != rounded_break_value)
        {
            ptrdiff_t increment = rounded_break_value - break_value;

            if (ve_sbrk(increment) == (void*)-1)
                goto done;
        }
    }

    if ((ptr = (uint8_t*)ve_sbrk((intptr_t)size)) == (void*)-1)
        return NULL;

    ret = ptr;

done:

    if (locked)
        ve_unlock(&_lock);

    return ret;
}

void* ve_alloc(size_t size)
{
    return ve_alloc_aligned(size, DEFAULT_ALIGNMENT);
}
