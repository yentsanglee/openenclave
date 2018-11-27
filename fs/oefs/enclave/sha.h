// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_SHA_H
#define _OEFS_SHA_H

#include "common.h"

OE_EXTERNC_BEGIN

typedef struct _oefs_sha256
{
    union {
        uint64_t words[4];
        uint8_t bytes[32];
    } u;
} oefs_sha256_t;

typedef struct _oefs_vector
{
    void* data;
    size_t size;
} oefs_vector_t;

int oefs_sha256(oefs_sha256_t* hash, const void* data, size_t size);

int oefs_sha256_v(
    oefs_sha256_t* hash,
    const oefs_vector_t* vector,
    size_t size);

void oefs_sha256_dump(const oefs_sha256_t* hash);

OE_INLINE bool oefs_sha256_eq(const oefs_sha256_t* x, const oefs_sha256_t* y)
{
    const uint64_t* p = x->u.words;
    const uint64_t* q = y->u.words;
    return p[0] == q[0] && p[1] == q[1] && p[2] == q[2] && p[3] == q[3];
}

OE_EXTERNC_END

#endif /* _OEFS_SHA_H */
