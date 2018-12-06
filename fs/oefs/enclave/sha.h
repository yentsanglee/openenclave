// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_SHA_H
#define _OEFS_SHA_H

#include "common.h"

OE_EXTERNC_BEGIN

typedef struct _oefs_sha256_context
{
    uint64_t __impl[16];
} oefs_sha256_context_t;

typedef struct _oefs_sha256
{
    uint8_t data[32];
} oefs_sha256_t;

int oefs_sha256_init(oefs_sha256_context_t* context);

int oefs_sha256_update(
    oefs_sha256_context_t* context, 
    const void* data, 
    size_t size);

void oefs_sha256_release(const oefs_sha256_context_t* context);

int oefs_sha256_finish(oefs_sha256_context_t* context, oefs_sha256_t* hash);

int oefs_sha256(oefs_sha256_t* hash, const void* data, size_t size);

void oefs_sha256_dump(const oefs_sha256_t* hash);

OE_EXTERNC_END

#endif /* _OEFS_SHA_H */
