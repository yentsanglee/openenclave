// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_SIGNAL_H
#define _VE_ENCLAVE_SIGNAL_H

#include "common.h"

#if defined(__GNUC__)
#define VE_SIGHUP 1
#define VE_SIGINT 2
#define VE_SIGQUIT 3
#define VE_SIGILL 4
#define VE_SIGTRAP 5
#define VE_SIGABRT 6
#define VE_SIGIOT VE_SIGABRT
#define VE_SIGBUS 7
#define VE_SIGFPE 8
#define VE_SIGKILL 9
#define VE_SIGUSR1 10
#define VE_SIGSEGV 11
#define VE_SIGUSR2 12
#define VE_SIGPIPE 13
#define VE_SIGALRM 14
#define VE_SIGTERM 15
#define VE_SIGSTKFLT 16
#define VE_SIGCHLD 17
#define VE_SIGCONT 18
#define VE_SIGSTOP 19
#define VE_SIGTSTP 20
#define VE_SIGTTIN 21
#define VE_SIGTTOU 22
#define VE_SIGURG 23
#define VE_SIGXCPU 24
#define VE_SIGXFSZ 25
#define VE_SIGVTALRM 26
#define VE_SIGPROF 27
#define VE_SIGWINCH 28
#define VE_SIGIO 29
#define VE_SIGPOLL 29
#define VE_SIGPWR 30
#define VE_SIGSYS 31
#define VE_SIGUNUSED VE_SIGSYS
#define VE_SIG_ERR ((void (*)(int)) - 1)
#define VE_SIG_DFL ((void (*)(int))0)
#define VE_SIG_IGN ((void (*)(int))1)
#endif

typedef struct _ve_sigset_t
{
    unsigned long __bits[128 / sizeof(long)];
} ve_sigset_t;

struct ve_sigaction
{
    void (*sa_handler)(int);
    ve_sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

int ve_sigaction(
    int signum,
    const struct ve_sigaction* act,
    struct ve_sigaction* oldact);

typedef void (*ve_sighandler_t)(int);

ve_sighandler_t ve_signal(int signum, ve_sighandler_t handler);

int ve_kill(int pid, int sig);

#endif /* _VE_ENCLAVE_SIGNAL_H */
