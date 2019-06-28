// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "signal.h"
#include "common.h"
#include "io.h"
#include "print.h"
#include "string.h"
#include "syscall.h"

#define _NSIG 65
#define SA_RESTART 0x10000000
#define SA_RESTORER 0x04000000
#define SA_RESETHAND 0x80000000

struct k_sigaction
{
    void (*handler)(int);
    unsigned long flags;
    void (*restorer)(void);
    unsigned mask[2];
    uint64_t padding[16];
};

extern void __ve_restore_rt(void);

int ve_sigaction(
    int signum,
    const struct ve_sigaction* act,
    struct ve_sigaction* oldact)
{
    __attribute__((aligned(16))) struct k_sigaction kact;
    __attribute__((aligned(16))) struct k_sigaction koldact;

    ve_memset(&kact, 0, sizeof(kact));
    ve_memset(&koldact, 0, sizeof(koldact));

    kact.handler = act->sa_handler;
    kact.flags = (unsigned long)(act->sa_flags | SA_RESTORER);
    kact.restorer = __ve_restore_rt;
#if 0
    ve_memcpy(&kact.mask, &act->sa_mask, _NSIG / 8);
#endif

    int rval = (int)ve_syscall4(
        VE_SYS_rt_sigaction, signum, (long)&kact, (long)&koldact, _NSIG / 8);

    if (oldact && rval == 0)
    {
        oldact->sa_handler = koldact.handler;
        oldact->sa_flags = (int)koldact.flags;
    }

    return rval;
}

typedef void (*ve_sighandler_t)(int);

ve_sighandler_t ve_signal(int signum, ve_sighandler_t handler)
{
    struct ve_sigaction act;
    struct ve_sigaction oldact;
    int rval;

    ve_memset(&act, 0, sizeof(act));
    ve_memset(&oldact, 0, sizeof(oldact));

    act.sa_handler = handler;
    act.sa_flags = SA_RESTART;

    if ((rval = ve_sigaction(signum, &act, &oldact)) != 0)
        return VE_SIG_ERR;

    return oldact.sa_handler;
}

int ve_kill(int pid, int sig)
{
    return (int)ve_syscall2(VE_SYS_kill, pid, sig);
}
