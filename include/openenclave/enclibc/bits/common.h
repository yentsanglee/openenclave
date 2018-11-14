// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_COMMON_H
#define _ENCLIBC_COMMON_H

#if defined(__cplusplus)
#define ENCLIBC_EXTERNC extern "C"
#define ENCLIBC_EXTERNC_BEGIN extern "C" {
#define ENCLIBC_EXTERNC_END }
#else
#define ENCLIBC_EXTERNC
#define ENCLIBC_EXTERNC_BEGIN
#define ENCLIBC_EXTERNC_END
#endif

#if defined(__GNUC__)
#define ENCLIBC_PRINTF_FORMAT(N, M) __attribute__((format(printf, N, M)))
#endif

#define ENCLIBC_INLINE static __inline

#ifndef NULL
#ifdef __cplusplus
#define NULL 0L
#else
#define NULL ((void*)0)
#endif
#endif

#define ENCLIBC_NEED_STDC_NAMES

ENCLIBC_EXTERNC_BEGIN

typedef __builtin_va_list enclibc_va_list;

typedef long ssize_t;
typedef unsigned long size_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long int64_t;
typedef unsigned long uint64_t;
typedef unsigned long uintptr_t;
typedef long ptrdiff_t;
typedef long time_t;
typedef long suseconds_t;
typedef int clockid_t;

#if defined(ENCLIBC_NEED_STDC_NAMES)

typedef enclibc_va_list va_list;

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

ENCLIBC_EXTERNC_END

#endif /* _ENCLIBC_COMMON_H */
