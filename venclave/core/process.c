// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "process.h"
#include "calls.h"
#include "elf.h"
#include "print.h"
#include "signal.h"
#include "socket.h"
#include "syscall.h"
#include "thread.h"
#include "time.h"

OE_NO_RETURN void ve_exit(int status)
{
    /* If this is a thread, then set the return value. */
    if (ve_getpid() != ve_gettid())
        ve_thread_set_retval(status);

    for (;;)
        ve_syscall1(VE_SYS_exit, status);
}

OE_NO_RETURN void ve_abort(void)
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
        ve_call0(ve_thread_get_sock(), VE_FUNC_ABORT, &retval);

    *((volatile int*)0) = 0;
    ve_exit(127);
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

int ve_fork(void)
{
    return (int)ve_syscall0(VE_SYS_fork);
}

int ve_execv(const char* path, char* const argv[])
{
    return (int)ve_syscall3(VE_SYS_execve, (long)path, (long)argv, (long)NULL);
}
