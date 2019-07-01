// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "syscall.h"

static long write(int fd, const void* buf, unsigned long count)
{
    return ve_syscall3(1, fd, (long)buf, (long)count);
}

void f(void);

static void exit(int status)
{
    ve_syscall1(60, status);
}

static int close(int fd)
{
    return (int)ve_syscall1(3, fd);
}

void* malloc(unsigned long size);

void free(void* ptr);

char* strcpy(char* dest, const char* src);

int main(void)
{
    char buf[1024];

    // strcpy(buf, "hello");

    f();
    write(1, "main()\n", 7);

    malloc(13);

    close(0);
    close(1);
    close(2);

    exit(0);

    return 0;
}
