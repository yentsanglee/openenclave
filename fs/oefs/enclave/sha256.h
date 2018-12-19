// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_SHA256_H
#define _OEFS_SHA256_H

#include "common.h"

OE_EXTERNC_BEGIN

typedef struct _sha256
{
    union
    {
        uint64_t u64[4];
        uint32_t u32[8];
        uint8_t data[32];
    }
    u;
} sha256_t;

OE_STATIC_ASSERT(sizeof(sha256_t) == 32);

int sha256(sha256_t* hash, const void* data, size_t size);

void sha256_64(sha256_t* hash, const void* data);

OE_INLINE bool sha256_eq(const sha256_t* x, const sha256_t* y)
{
    return 
        x->u.u64[0] == y->u.u64[0] &&
        x->u.u64[1] == y->u.u64[1] &&
        x->u.u64[2] == y->u.u64[2] &&
        x->u.u64[3] == y->u.u64[3];
}

OE_INLINE void sha256_copy(sha256_t* dest, const sha256_t* src)
{
        dest->u.u64[0] = src->u.u64[0];
        dest->u.u64[1] = src->u.u64[1];
        dest->u.u64[2] = src->u.u64[2];
        dest->u.u64[3] = src->u.u64[3];
}

OE_EXTERNC_END

#endif /* _OEFS_SHA256_H */
