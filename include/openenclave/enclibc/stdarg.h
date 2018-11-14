// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_STDARG_H
#define _ENCLIBC_STDARG_H

#include "bits/common.h"

#if defined(_MSC_VER)
typedef char* enclibc_va_list;
#define enclibc_va_start(ap, x) __va_start(&ap, x)
#define enclibc_va_arg(ap, type) *((type*)((ap += sizeof(__int64)) - sizeof(__int64)))
#define enclibc_va_end(ap) (ap = (va_list)0)
#define enclibc_va_copy(ap1, ap2) (ap1 = ap2)
#elif defined(__linux__)
#define enclibc_va_start __builtin_va_start
#define enclibc_va_arg __builtin_va_arg
#define enclibc_va_end __builtin_va_end
#define enclibc_va_copy __builtin_va_copy
#endif

#if defined(ENCLIBC_NEED_STDC_NAMES)

#define va_start enclibc_va_start
#define va_arg enclibc_va_arg
#define va_end enclibc_va_end
#define va_copy enclibc_va_copy

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

#endif /* _ENCLIBC_STDARG_H */
