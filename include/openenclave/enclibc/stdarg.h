// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_STDARG_H
#define _ENCLIBC_STDARG_H

#include "bits/common.h"

#define enclibc_va_start __builtin_va_start
#define enclibc_va_arg __builtin_va_arg
#define enclibc_va_end __builtin_va_end
#define enclibc_va_copy __builtin_va_copy

#if defined(ENCLIBC_NEED_STDC_NAMES)

#define va_start enclibc_va_start
#define va_arg enclibc_va_arg
#define va_end enclibc_va_end
#define va_copy enclibc_va_copy

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

#endif /* _ENCLIBC_STDARG_H */
