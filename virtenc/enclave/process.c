// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "process.h"
#include "globals.h"
#include "print.h"
#include "syscall.h"

void ve_exit(int status)
{
    for (;;)
        ve_syscall1(OE_SYS_exit, status);
}

__attribute__((__noreturn__)) void ve_abort(void)
{
    *((volatile int*)0) = 0;
    ve_exit(127);

    for (;;)
        ;
}

__attribute__((__noreturn__)) void ve_panic(const char* msg)
{
    ve_puts(msg);
    ve_abort();
}

int ve_gettid(void)
{
    return (int)ve_syscall0(OE_SYS_gettid);
}

int ve_getpid(void)
{
    return (int)ve_syscall0(OE_SYS_getpid);
}

int ve_waitpid(int pid, int* status, int options)
{
    return (int)ve_syscall4(OE_SYS_wait4, pid, (long)status, options, 0);
}

void* ve_get_baseaddr(void)
{
    return (uint8_t*)&__ve_self - __ve_self;
}
