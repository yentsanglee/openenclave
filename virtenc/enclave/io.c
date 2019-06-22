// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "io.h"
#include "syscall.h"

int ve_close(int fd)
{
    return ve_syscall1(OE_SYS_close, (long)fd);
}

ssize_t ve_read(int fd, void* buf, size_t count)
{
    return ve_syscall3(OE_SYS_read, fd, (long)buf, (long)count);
}

ssize_t ve_write(int fd, const void* buf, size_t count)
{
    return ve_syscall3(OE_SYS_write, fd, (long)buf, (long)count);
}
