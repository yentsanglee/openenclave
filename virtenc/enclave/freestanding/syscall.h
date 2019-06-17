// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FREESTANDING_SYSCALL_H
#define _FREESTANDING_SYSCALL_H

#include "../freestanding/defs.h"
#include "../freestanding/types.h"

#if defined(__x86_64__)
#include "../freestanding/bits/syscall_x86_64.h"
#else
#error "unsupported"
#endif

FS_INLINE long fs_syscall0(long n)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n)
                         : "rcx", "r11", "memory");
    return ret;
}

FS_INLINE long fs_syscall1(long n, long x1)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1)
                         : "rcx", "r11", "memory");

    return ret;
}

FS_INLINE long fs_syscall2(long n, long x1, long x2)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1), "S"(x2)
                         : "rcx", "r11", "memory");

    return ret;
}

FS_INLINE long fs_syscall3(long n, long x1, long x2, long x3)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1), "S"(x2), "d"(x3)
                         : "rcx", "r11", "memory");

    return ret;
}

FS_INLINE long fs_syscall4(long n, long x1, long x2, long x3, long x4)
{
    unsigned long ret;
    register long r10 __asm__("r10") = x4;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1), "S"(x2), "d"(x3), "r"(r10)
                         : "rcx", "r11", "memory");

    return ret;
}

FS_INLINE long syscall5(long n, long x1, long x2, long x3, long x4, long x5)
{
    unsigned long ret;
    register long r10 __asm__("r10") = x4;
    register long r8 __asm__("r8") = x5;
    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1), "S"(x2), "d"(x3), "r"(r10), "r"(r8)
                         : "rcx", "r11", "memory");

    return ret;
}

FS_INLINE long
syscall6(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    unsigned long ret;
    register long r10 __asm__("r10") = x4;
    register long r8 __asm__("r8") = x5;
    register long r9 __asm__("r9") = x6;

    __asm__ __volatile__(
        "syscall"
        : "=a"(ret)
        : "a"(n), "D"(x1), "S"(x2), "d"(x3), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory");

    return ret;
}

#endif /* _FREESTANDING_SYSCALL_H */
