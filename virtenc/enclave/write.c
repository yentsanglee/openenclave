// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include "syscall.h"

ssize_t ve_write(int fd, const void* buf, size_t count)
{
    return ve_syscall(OE_SYS_write, fd, (long)buf, (long)count);
}
