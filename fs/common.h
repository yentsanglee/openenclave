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

#ifdef __cplusplus
#define FS_EXTERN_C_BEGIN extern "C" {
#define FS_EXTERN_C_END }
#else
#define FS_EXTERN_C_BEGIN
#define FS_EXTERN_C_END
#endif

#ifdef __GNUC__
#define FS_UNUSED_ATTRIBUTE __attribute__((unused))
#elif _MSC_VER
#define FS_UNUSED_ATTRIBUTE
#else
#error FS_UNUSED_ATTRIBUTE not implemented
#endif

#define __FS_CONCAT(X, Y) X##Y
#define FS_CONCAT(X, Y) __FS_CONCAT(X, Y)

#define FS_STATIC_ASSERT(COND)       \
    typedef unsigned char FS_CONCAT( \
        __FS_STATIC_ASSERT, __LINE__)[(COND) ? 1 : -1] FS_UNUSED_ATTRIBUTE

#define FS_FIELD_SIZE(TYPE, FIELD) (sizeof(((TYPE*)0)->FIELD))

#endif /* _FS_DEFS_H */
