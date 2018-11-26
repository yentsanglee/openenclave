// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_UTILS_H
#define _OEFS_UTILS_H

#include "common.h"

OE_EXTERNC_BEGIN

OE_INLINE size_t oefs_round_to_multiple(size_t x, size_t m)
{
    return (size_t)((x + (m - 1)) / m * m);
}

OE_INLINE bool oefs_is_pow_of_2(size_t n)
{
    return (n & (n - 1)) == 0;
}

OE_INLINE size_t oefs_min_size(size_t x, size_t y)
{
    return (x < y) ? x : y;
}

OE_EXTERNC_END

#endif /* _OEFS_UTILS_H */
