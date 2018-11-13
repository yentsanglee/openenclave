// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_UTILS_H
#define _FS_UTILS_H

#include "common.h"

FS_EXTERN_C_BEGIN

FS_INLINE size_t fs_round_to_multiple(size_t x, size_t m)
{
    return (size_t)((x + (m - 1)) / m * m);
}

FS_INLINE bool fs_is_pow_of_2(size_t n)
{
    return (n & (n - 1)) == 0;
}

FS_INLINE size_t fs_min_size(size_t x, size_t y)
{
    return (x < y) ? x : y;
}

FS_EXTERN_C_END

#endif /* _FS_UTILS_H */
