// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "process.h"
#include "call.h"
#include "globals.h"
#include "print.h"
#include "signal.h"
#include "syscall.h"
#include "thread.h"
#include "time.h"

VE_NO_RETURN void ve_exit(int status)
{
    /* If this is a thread, then set the return value. */
    if (ve_getpid() != ve_gettid())
        ve_thread_set_retval(status);

    for (;;)
        ve_syscall1(VE_SYS_exit, status);
}

VE_NO_RETURN void ve_abort(void)
{
    uint64_t retval = 0;
    static bool _abort_called;
    static ve_lock_t _lock;
    bool abort = false;

    ve_lock(&_lock);
    {
        if (!_abort_called)
        {
            abort = true;
            _abort_called = true;
        }
    }
    ve_unlock(&_lock);

    /* Ask the host to call abort(). */
    if (abort)
        ve_call0(__ve_thread_sock_tls, VE_FUNC_ABORT, &retval);

    *((volatile int*)0) = 0;
    ve_exit(127);
}

VE_NO_RETURN void ve_panic(const char* msg)
{
    ve_puts(msg);
    ve_abort();
}

int ve_gettid(void)
{
    return (int)ve_syscall0(VE_SYS_gettid);
}

int ve_getpid(void)
{
    return (int)ve_syscall0(VE_SYS_getpid);
}

int ve_waitpid(int pid, int* status, int options)
{
    return (int)ve_syscall4(VE_SYS_wait4, pid, (long)status, options, 0);
}

void* ve_get_baseaddr(void)
{
    return (uint8_t*)&__ve_self - __ve_self;
}

#if 0

void ve_call_fini_functions(void)
{
    void (**fn)(void);
    extern void (*__fini_array_start)(void);
    extern void (*__fini_array_end)(void);

    for (fn = &__fini_array_end - 1; fn >= &__fini_array_start; fn--)
    {
        (*fn)();
    }
}

void __libc_csu_fini(void)
{
    ve_call_fini_functions();
}

void ve_call_init_functions(void)
{
    void (**fn)(void);
    extern void (*__init_array_start)(void);
    extern void (*__init_array_end)(void);

    for (fn = &__init_array_start; fn < &__init_array_end; fn++)
        (*fn)();
}

void __libc_csu_init(void)
{
    ve_call_init_functions();
}

#endif
