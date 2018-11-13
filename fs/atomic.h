// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_ATOMIC_H
#define _FS_ATOMIC_H

#include "common.h"

#if defined(_MSC_VER)
#include <windows.h>
#endif

FS_EXTERN_C_BEGIN

/* Atomically increment x and return its new value. */
FS_INLINE uint64_t fs_atomic_increment(volatile uint64_t* x)
{
#if defined(__GNUC__)
    return __sync_add_and_fetch(x, 1);
#elif defined(_MSC_VER)
    return InterlockedIncrement64(x);
#endif
}

/* Atomically decrement x and return its new value. */
FS_INLINE uint64_t fs_atomic_decrement(volatile uint64_t* x)
{
#if defined(__GNUC__)
    return __sync_sub_and_fetch(x, 1);
#elif defined(_MSC_VER)
    return InterlockedDecrement64(x);
#endif
}

FS_EXTERN_C_END

#endif /* _FS_ATOMIC_H */
