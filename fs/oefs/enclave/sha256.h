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
        uint64_t words[4];
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
        x->u.words[0] == y->u.words[0] &&
        x->u.words[1] == y->u.words[1] &&
        x->u.words[2] == y->u.words[2] &&
        x->u.words[3] == y->u.words[3];
}

OE_EXTERNC_END

#endif /* _OEFS_SHA256_H */
