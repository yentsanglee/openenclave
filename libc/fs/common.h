// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_DEFS_H
#define _FS_DEFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FS_PATH_MAX 256

#define FS_BLOCK_SIZE 512

#define FS_INLINE static __inline

#define FS_COUNTOF(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#define FS_TRACE printf("%s(%u): %s()\n", __FILE__, __LINE__, __FUNCTION__)

#endif /* _FS_DEFS_H */
