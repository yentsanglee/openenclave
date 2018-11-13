// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_SHA_H
#define _FS_SHA_H

#include "common.h"

FS_EXTERN_C_BEGIN

typedef struct _fs_sha256
{
    union {
        uint64_t words[4];
        uint8_t bytes[32];
    } u;
} fs_sha256_t;

typedef struct _fs_vector
{
    void* data;
    size_t size;
} fs_vector_t;

int fs_sha256(fs_sha256_t* hash, const void* data, size_t size);

int fs_sha256_v(fs_sha256_t* hash, const fs_vector_t* vector, size_t size);

void fs_sha256_dump(const fs_sha256_t* hash);

FS_INLINE bool fs_sha256_eq(const fs_sha256_t* x, const fs_sha256_t* y)
{
    return x->u.words[0] == y->u.words[0] && x->u.words[1] == y->u.words[1] &&
           x->u.words[2] == y->u.words[2] && x->u.words[3] == y->u.words[3];
}

FS_EXTERN_C_END

#endif /* _FS_SHA_H */
