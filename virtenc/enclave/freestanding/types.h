// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FREESTANDING_TYPES_H
#define _FREESTANDING_TYPES_H

#include "../freestanding/defs.h"

#define FS_INT8_MIN (-1 - 0x7f)
#define FS_INT8_MAX (0x7f)
#define FS_UINT8_MAX (0xff)
#define FS_INT16_MIN (-1 - 0x7fff)
#define FS_INT16_MAX (0x7fff)
#define FS_UINT16_MAX (0xffff)
#define FS_INT32_MIN (-1 - 0x7fffffff)
#define FS_INT32_MAX (0x7fffffff)
#define FS_UINT32_MAX (0xffffffffu)
#define FS_INT64_MIN (-1 - 0x7fffffffffffffff)
#define FS_INT64_MAX (0x7fffffffffffffff)
#define FS_UINT64_MAX (0xffffffffffffffffu)
#define FS_SIZE_MAX FS_UINT64_MAX
#define FS_SSIZE_MAX FS_INT64_MAX

#ifndef NULL
#ifdef __cplusplus
#define NULL 0L
#else
#define NULL ((void*)0)
#endif
#endif

typedef unsigned long size_t;
typedef long ssize_t;

typedef unsigned char uint8_t;
typedef char int8_t;

typedef unsigned short uint16_t;
typedef short int16_t;

typedef unsigned int uint32_t;
typedef int int32_t;

typedef unsigned long uint64_t;
typedef long int64_t;

#endif /* _FREESTANDING_TYPES_H */
