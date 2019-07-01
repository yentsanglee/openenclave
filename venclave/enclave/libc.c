// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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

void __libc_csu_fini(void)
{
    void (**fn)(void);
    extern void (*__fini_array_start)(void);
    extern void (*__fini_array_end)(void);

    for (fn = &__fini_array_end - 1; fn >= &__fini_array_start; fn--)
    {
        (*fn)();
    }
}

void __libc_csu_init(void)
{
    void (**fn)(void);
    extern void (*__init_array_start)(void);
    extern void (*__init_array_end)(void);

    for (fn = &__init_array_start; fn < &__init_array_end; fn++)
        (*fn)();
}

void __libc_start_main(void)
{
    int exit_status;
    extern int main(void);

    __libc_csu_init();
    exit_status = main();
    __libc_csu_fini();

    _exit(exit_status);
}

void __stack_chk_fail(void)
{
}
