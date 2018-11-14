// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_ASSERT_H
#define _ENCLIBC_ASSERT_H

#include "bits/common.h"

void __enclibc_assert_fail(
    const char* expr,
    const char* file,
    int line,
    const char* func);

// clang-format off
#ifndef NDEBUG
#define enclibc_assert(EXPR)                                                \
    do                                                                      \
    {                                                                       \
        if (!(EXPR))                                                        \
            __enclibc_assert_fail(#EXPR, __FILE__, __LINE__, __FUNCTION__); \
    } while (0)
#else
#define enclibc_assert(EXPR)
#endif
// clang-format on

#if defined(ENCLIBC_NEED_STDC_NAMES)

#define assert(EXPR) enclibc_assert(EXPR)

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

#endif /* _ENCLIBC_ASSERT_H */
