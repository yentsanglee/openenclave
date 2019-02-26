// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/limits.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/string.h>

extern char __oe_cwd[OE_PATH_MAX];

static const long SYS_getcwd = 79;

OE_INLINE long syscall(long n, char* buf, size_t size)
{
    if (n != SYS_getcwd || !buf || !size)
        return -1;

    OE_UNUSED(n);

    oe_strlcpy(buf, __oe_cwd, size);
    return (long)size;
}

#define getcwd oe_getcwd
#define strdup oe_strdup
#define PATH_MAX OE_PATH_MAX
#define errno oe_errno

#include "../../3rdparty/musl/musl/src/unistd/getcwd.c"
