// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_STDINT_H
#define _ENCLIBC_STDINT_H

#include "bits/common.h"

#define ENCLIBC_INT8_MIN (-1 - 0x7f)
#define ENCLIBC_INT8_MAX (0x7f)
#define ENCLIBC_UINT8_MAX (0xff)
#define ENCLIBC_INT16_MIN (-1 - 0x7fff)
#define ENCLIBC_INT16_MAX (0x7fff)
#define ENCLIBC_UINT16_MAX (0xffff)
#define ENCLIBC_INT32_MIN (-1 - 0x7fffffff)
#define ENCLIBC_INT32_MAX (0x7fffffff)
#define ENCLIBC_UINT32_MAX (0xffffffffu)
#define ENCLIBC_INT64_MIN (-1 - 0x7fffffffffffffff)
#define ENCLIBC_INT64_MAX (0x7fffffffffffffff)
#define ENCLIBC_UINT64_MAX (0xffffffffffffffffu)
#define ENCLIBC_SIZE_MAX ENCLIBC_UINT64_MAX

#if defined(ENCLIBC_NEED_STDC_NAMES)

#define INT8_MIN ENCLIBC_INT8_MIN
#define INT8_MAX ENCLIBC_INT8_MAX
#define UINT8_MAX ENCLIBC_UINT8_MAX
#define INT16_MIN ENCLIBC_INT16_MIN
#define INT16_MAX ENCLIBC_INT16_MAX
#define UINT16_MAX ENCLIBC_UINT16_MAX
#define INT32_MIN ENCLIBC_INT32_MIN
#define INT32_MAX ENCLIBC_INT32_MAX
#define UINT32_MAX ENCLIBC_UINT32_MAX
#define INT64_MIN ENCLIBC_INT64_MIN
#define INT64_MAX ENCLIBC_INT64_MAX
#define UINT64_MAX ENCLIBC_UINT64_MAX
#define SIZE_MAX ENCLIBC_SIZE_MAX

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

#endif /* _ENCLIBC_STDINT_H */
