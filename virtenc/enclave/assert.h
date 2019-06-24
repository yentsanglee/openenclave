// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_ASSERT_H
#define _VE_ENCLAVE_ASSERT_H

#define assert ve_assert

#define __assert_fail __ve_assert_fail

void __ve_assert_fail(
    const char* expr,
    const char* file,
    int line,
    const char* function);

#ifndef NDEBUG
#define ve_assert(EXPR)                                                \
    do                                                                 \
    {                                                                  \
        if (!(EXPR))                                                   \
            __ve_assert_fail(#EXPR, __FILE__, __LINE__, __FUNCTION__); \
    } while (0)
#else
#define ve_assert(EXPR)
#endif

#endif /* _VE_ENCLAVE_ASSERT_H */
