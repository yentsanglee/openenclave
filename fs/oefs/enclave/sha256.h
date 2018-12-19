// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_SHA256_H
#define _OEFS_SHA256_H

#include "common.h"

OE_EXTERNC_BEGIN

typedef struct _sha256
{
    uint8_t data[32];
} sha256_t;

int sha256(sha256_t* hash, const void* data, size_t size);

OE_INLINE bool sha256_eq(const sha256_t* x, const sha256_t* y)
{
    return memcmp(x, y, sizeof(sha256_t)) == 0;
}

OE_EXTERNC_END

#endif /* _OEFS_SHA256_H */
