// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "syscall.h"

long __write(int fd, const void* buf, unsigned long count)
{
    return ve_syscall3(1, fd, (long)buf, (long)count);
}

void f(void);

void __exit(int status)
{
    ve_syscall1(60, status);
}

void* malloc(unsigned long size);

void free(void* ptr);

int main(void)
{
    f();
    __write(1, "main()\n", 7);

    malloc(1);
    malloc(7);
    malloc(13);

    __exit(0);

    return 0;
}
