// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_COMMON_H
#define _OEFS_COMMON_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/atomic.h>
#include <openenclave/internal/defs.h>
#include <stdlib.h>
#include <string.h>
#include "../common/oefs.h"

#define OEFS_PATH_MAX 256

typedef struct _oefs_blk
{
    uint8_t data[OEFS_BLOCK_SIZE];
} oefs_blk_t;

#endif /* _OEFS_COMMON_H */
