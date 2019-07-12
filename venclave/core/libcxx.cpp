// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "syscall.h"

static long _syscall1(long n, long x1)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1)
                         : "rcx", "r11", "memory");

    return (long)ret;
}

static void _exit(int status)
{
    static int SYS_EXIT = 60;

    for (;;)
        _syscall1(SYS_EXIT, status);
}

static void _abort(void)
{
    *((volatile int*)0) = 0;
    _exit(127);
}

static long _syscall3(long n, long x1, long x2, long x3)
{
    unsigned long ret;

    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(x1), "S"(x2), "d"(x3)
                         : "rcx", "r11", "memory");

    return (long)ret;
}

static void _put(const char* s, size_t n)
{
    _syscall3(VE_SYS_write, 2, (long)s, (long)n);
}

#if 0
extern "C" void __libc_csu_fini(void)
{
    void (**fn)(void);
    extern void (*__fini_array_start)(void);
    extern void (*__fini_array_end)(void);

    for (fn = &__fini_array_end - 1; fn >= &__fini_array_start; fn--)
    {
        (*fn)();
    }
}
#endif

#if 0
extern "C" void __libc_csu_init(void)
{
    void (**fn)(void);
    extern void (*__init_array_start)(void);
    extern void (*__init_array_end)(void);

    for (fn = &__init_array_start; fn < &__init_array_end; fn++)
        (*fn)();
}
#endif

#if 0
extern "C" void __libc_start_main(void)
{
    int exit_status;
    extern int main(void);

    //    __libc_csu_init();
    exit_status = main();
    __libc_csu_fini();

    _exit(exit_status);
}
#endif

#if 0
extern "C" void __stack_chk_fail(void)
{
}
#endif

extern "C" void __cxx_global_var_init(void)
{
    const char MSG[] = "__cxx_global_var_init panic\n";
    _put(MSG, sizeof(MSG) - 1);
    _abort();
    (void)_abort;
}

extern "C" void __cxa_thread_atexit(void)
{
#if 0
    const char MSG[] = "__cxa_thread_atexit panic\n";
    _put(MSG, sizeof(MSG) - 1);
    _abort();
#endif
}
