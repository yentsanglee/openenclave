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
#include <stdarg.h>

#define FS_PATH_MAX 256

#define FS_BLOCK_SIZE 1024

#define FS_INLINE static __inline

#define FS_COUNTOF(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

// clang-format off
#define FS_TRACE \
    do \
    { \
        printf("%s(%u): %s()\n", __FILE__, __LINE__, __FUNCTION__); \
        fflush(stdout); \
    } \
    while (0)
// clang-format on

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

#if defined(__GNUC__)
#define FS_PACK_BEGIN _Pragma("pack(push, 1)")
#define FS_PACK_END _Pragma("pack(pop)")
#elif _MSC_VER
#define FS_PACK_BEGIN __pragma(pack(push, 1))
#define FS_PACK_END __pragma(pack(pop))
#endif

#endif /* _FS_DEFS_H */
