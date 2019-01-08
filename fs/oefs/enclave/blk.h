// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_BLK_H
#define _OEFS_BLK_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/defs.h>
#include <openenclave/internal/oefs.h>

typedef struct _oefs_blk
{
    union {
        uint64_t u64[OEFS_BLOCK_SIZE / sizeof(uint64_t)];
        uint8_t data[OEFS_BLOCK_SIZE];
    } u;
} oefs_blk_t;

OE_STATIC_ASSERT(sizeof(oefs_blk_t) == OEFS_BLOCK_SIZE);

#if 1
OE_INLINE void oefs_blk_copy(oefs_blk_t* dest, const oefs_blk_t* src)
{
    for (size_t i = 0; i < OEFS_BLOCK_SIZE / sizeof(uint64_t); i++)
        dest->u.u64[i] = src->u.u64[i];
}
#else
void oefs_blk_copy(oefs_blk_t* dest, const oefs_blk_t* src);
#endif

OE_INLINE void oefs_blk_clear(oefs_blk_t* blk)
{
    for (size_t i = 0; i < OEFS_BLOCK_SIZE / sizeof(uint64_t); i++)
        blk->u.u64[i] = 0;
}

OE_INLINE bool oefs_blk_equal(const oefs_blk_t* b1, const oefs_blk_t* b2)
{
    const uint64_t* p = b1->u.u64;
    const uint64_t* q = b2->u.u64;
    size_t n = OEFS_BLOCK_SIZE / sizeof(uint64_t);

    while (n && *p == *q)
    {
        n--;
        p++;
        q++;
    }

    return n == 0;
}

#endif /* _OEFS_BLK_H */
