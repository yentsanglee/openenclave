#include "syscall.h"
#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/corelibc/stdarg.h>

static long _syscall0(long n)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n)
                         : "rcx", "r11", "memory");
    return ret;
}

static long _syscall1(long n, long x1)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1)
                         : "rcx", "r11", "memory");

    return ret;
}

static long _syscall2(long n, long x1, long x2)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1), "S"(x2)
                         : "rcx", "r11", "memory");

    return ret;
}

static long _syscall3(long n, long x1, long x2, long x3)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1), "S"(x2), "d"(x3)
                         : "rcx", "r11", "memory");

    return ret;
}

static long _syscall4(long n, long x1, long x2, long x3, long x4)
{
    unsigned long ret;
    register long r10 __asm__("r10") = x4;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1), "S"(x2), "d"(x3), "r"(r10)
                         : "rcx", "r11", "memory");

    return ret;
}

static long _syscall5(long n, long x1, long x2, long x3, long x4, long x5)
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

static long
_syscall6(long n, long x1, long x2, long x3, long x4, long x5, long x6)
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

static long
_syscall(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    switch (n)
    {
        case OE_SYS_read:
            return _syscall3(n, x1, x2, x3);
        case OE_SYS_write:
            return _syscall3(n, x1, x2, x3);
        case OE_SYS_exit:
            return _syscall1(n, x1);
        default:
            return -1;
    }
}

long ve_syscall(long number, ...)
{
    oe_va_list ap;

    oe_va_start(ap, number);
    long x1 = oe_va_arg(ap, long);
    long x2 = oe_va_arg(ap, long);
    long x3 = oe_va_arg(ap, long);
    long x4 = oe_va_arg(ap, long);
    long x5 = oe_va_arg(ap, long);
    long x6 = oe_va_arg(ap, long);
    long ret = _syscall(number, x1, x2, x3, x4, x5, x6);
    oe_va_end(ap);

    (void)_syscall0;
    (void)_syscall1;
    (void)_syscall2;
    (void)_syscall3;
    (void)_syscall4;
    (void)_syscall5;
    (void)_syscall6;

    return ret;
}
