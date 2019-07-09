// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "io.h"
#include "common.h"
#include "syscall.h"

ssize_t ve_read(int fd, void* buf, size_t count)
{
    return ve_syscall3(VE_SYS_read, fd, (long)buf, (long)count);
}

ssize_t ve_write(int fd, const void* buf, size_t count)
{
    return ve_syscall3(VE_SYS_write, fd, (long)buf, (long)count);
}

int ve_close(int fd)
{
    return (int)ve_syscall1(VE_SYS_close, fd);
}

int ve_ioctl(int fd, unsigned long request, ...)
{
    long x1;
    long x2;
    long x3;
    long x4;
    long x5;
    long x6;

    ve_va_list ap;
    ve_va_start(ap, request);
    x1 = (long)fd;
    x2 = (long)request;
    x3 = (long)ve_va_arg(ap, long);
    x4 = (long)ve_va_arg(ap, long);
    x5 = (long)ve_va_arg(ap, long);
    x6 = (long)ve_va_arg(ap, long);
    ve_va_end(ap);

    return (int)ve_syscall6(VE_SYS_ioctl, x1, x2, x3, x4, x5, x6);
}

int ve_pipe(int pipefd[2])
{
    return (int)ve_syscall1(VE_SYS_pipe, (long)pipefd);
}

int ve_dup(int fd)
{
    return (int)ve_syscall1(VE_SYS_dup, fd);
}

int ve_getdtablesize(void)
{
    int ret = -1;
    const int RLIMIT_NOFILE = 7;
    unsigned long rlim[2];

    if (ve_syscall2(VE_SYS_getrlimit, RLIMIT_NOFILE, (long)rlim) < 0)
        goto done;

    ret = (int)rlim[0];

done:
    return ret;
}

#include "../common/io.c"
