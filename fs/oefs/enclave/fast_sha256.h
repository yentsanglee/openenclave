// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_FAST_SHA256_H
#define _OEFS_FAST_SHA256_H

#include "common.h"

OE_EXTERNC_BEGIN

int fast_sha256(uint8_t hash[32], const void* data, size_t size);

OE_EXTERNC_END

#endif /* _OEFS_FAST_SHA256_H */
