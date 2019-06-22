// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "io.h"
#include <openenclave/corelibc/stdarg.h>
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

int ve_ioctl(int fd, unsigned long request, ...)
{
    int ret = -1;
    long x1;
    long x2;
    long x3;
    long x4;
    long x5;
    long x6;

    oe_va_list ap;
    oe_va_start(ap, request);
    x1 = (long)fd;
    x2 = (long)request;
    x3 = (long)oe_va_arg(ap, long);
    x4 = (long)oe_va_arg(ap, long);
    x5 = (long)oe_va_arg(ap, long);
    x6 = (long)oe_va_arg(ap, long);
    oe_va_end(ap);

    ret = ve_syscall6(OE_SYS_ioctl, x1, x2, x3, x4, x5, x6);

    return ret;
}
