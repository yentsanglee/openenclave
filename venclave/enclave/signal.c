// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "signal.h"
#include "common.h"
#include "io.h"
#include "print.h"
#include "string.h"
#include "syscall.h"

/* Defined in restore.S */
extern void __ve_restore_rt(void);

ve_sighandler_t ve_signal(int signum, ve_sighandler_t handler)
{
    const unsigned long NSIG = 65;
    const unsigned long SA_RESTART = 0x10000000;
    const unsigned long SA_RESTORER = 0x04000000;
    struct sigaction
    {
        void (*handler)(int);
        unsigned long flags;
        void (*restorer)(void);
        unsigned mask[2];
        uint64_t padding[16];
    };
    __attribute__((aligned(16))) struct sigaction act;
    __attribute__((aligned(16))) struct sigaction oldact;
    long rval;

    ve_memset(&act, 0, sizeof(act));
    ve_memset(&oldact, 0, sizeof(oldact));

    act.handler = handler;
    act.flags = SA_RESTART | SA_RESTORER;
    act.restorer = __ve_restore_rt;

    rval = ve_syscall4(
        VE_SYS_rt_sigaction, signum, (long)&act, (long)&oldact, NSIG / 8);

    if (rval != 0)
        return VE_SIG_ERR;

    return oldact.handler;
}

int ve_kill(int pid, int sig)
{
    return (int)ve_syscall2(VE_SYS_kill, pid, sig);
}

int ve_tkill(int tid, int sig)
{
    return (int)ve_syscall2(VE_SYS_tkill, tid, sig);
}

int ve_tgkill(int tgid, int tid, int sig)
{
    return (int)ve_syscall3(VE_SYS_tgkill, tgid, tid, sig);
}
