// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_SHA_H
#define _FS_SHA_H

#include "common.h"

typedef struct _fs_sha256
{
    uint8_t data[32];
} fs_sha256_t;

int fs_sha256(fs_sha256_t* hash, const void* data, size_t size);

void fs_sha256_dump(const fs_sha256_t* hash);

#endif /* _FS_SHA_H */
