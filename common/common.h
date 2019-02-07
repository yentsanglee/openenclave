// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_COMMON_COMMON_H
#define _OE_COMMON_COMMON_H

// This file is intended to be used by code that exists in common folder.
// It includes the necessary header files and allows code to be written against
// standard C library.
// When compiling for enclave, it routes the C library calls to enclave core
// libc inline functions.

#ifdef OE_BUILD_ENCLAVE

#include <openenclave/corelibc/stdint.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>

#else

#include <openenclave/host.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

OE_INLINE void* oe_malloc(size_t size)
{
    return malloc(size);
}

OE_INLINE void oe_free(void* ptr)
{
    return free(ptr);
}

OE_INLINE size_t oe_strlen(const char* s)
{
    return strlen(s);
}

#endif

#endif // _OE_COMMON_COMMON_H
