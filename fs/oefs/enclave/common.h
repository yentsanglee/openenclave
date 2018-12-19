// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_COMMON_H
#define _OEFS_COMMON_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/atomic.h>
#include <openenclave/internal/defs.h>
#include <openenclave/internal/oefs.h>
#include <stdlib.h>
#include <string.h>

#define OEFS_PATH_MAX 256

typedef struct _oefs_blk
{
    union
    {
        uint64_t words[OEFS_BLOCK_SIZE / sizeof(uint64_t)];
        uint8_t data[OEFS_BLOCK_SIZE];
    }
    u;
} oefs_blk_t;

OE_INLINE void oefs_blk_copy(oefs_blk_t* dest, const oefs_blk_t* src)
{
    for (size_t i = 0; i < OEFS_BLOCK_SIZE / sizeof(uint64_t); i++)
        dest->u.words[i] = src->u.words[i];
}

OE_INLINE void oefs_blk_clear(oefs_blk_t* blk)
{
    for (size_t i = 0; i < OEFS_BLOCK_SIZE / sizeof(uint64_t); i++)
        blk->u.words[i] = 0;
}

OE_INLINE bool oefs_blk_equal(const oefs_blk_t* b1, const oefs_blk_t* b2)
{
    const uint64_t* p = b1->u.words;
    const uint64_t* q = b2->u.words;
    size_t n = OEFS_BLOCK_SIZE / sizeof(uint64_t);

    while (n-- && *p++ == *q++)
        ;

    return n == 0;
}

#endif /* _OEFS_COMMON_H */
