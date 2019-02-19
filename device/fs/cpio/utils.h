// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_UTILS_H
#define _OE_UTILS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

OE_INLINE size_t oe_round_to_multiple_size_t(size_t x, size_t m)
{
    return (size_t)((x + (m - 1)) / m * m);
}

OE_INLINE long oe_round_to_multiple_long(long x, long m)
{
    return (long)((x + (m - 1)) / m * m);
}

OE_INLINE bool oe_is_pow_of_2(size_t n)
{
    return (n & (n - 1)) == 0;
}

OE_INLINE size_t oe_min_size(size_t x, size_t y)
{
    return (x < y) ? x : y;
}

OE_EXTERNC_END

#endif /* _OE_UTILS_H */
