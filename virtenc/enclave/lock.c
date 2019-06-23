// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "lock.h"
#include "print.h"
#include "syscall.h"

#define FUTEX_PRIVATE_FLAG 128

#define a_cas __sync_val_compare_and_swap
#define a_fetch_add __sync_fetch_and_add

#define INT_MIN OE_INT_MIN

struct _libc
{
    bool threads_minus_1;
};

static struct _libc libc = {true};

static int __futexwait(volatile int* uaddr, int val, int priv)
{
    const int FUTEX_WAIT = 0;
    int futex_op = FUTEX_WAIT;

    if (priv)
        futex_op |= FUTEX_PRIVATE_FLAG;

    return (int)ve_syscall6(OE_SYS_futex, (long)uaddr, futex_op, val, 0, 0, 0);
}

static int __wake(volatile int* uaddr, int val, int priv)
{
    const int FUTEX_WAKE = 1;
    int futex_op = FUTEX_WAKE;

    if (priv)
        futex_op |= FUTEX_PRIVATE_FLAG;

    return (int)ve_syscall6(OE_SYS_futex, (long)uaddr, futex_op, val, 0, 0, 0);
}

#define __lock __ve_lock
#define __unlock __ve_unlock

#include "../../3rdparty/musl/musl/src/thread/__lock.c"

void ve_lock(ve_lock_t* lock)
{
    return __ve_lock(lock);
}

void ve_unlock(ve_lock_t* lock)
{
    return __ve_unlock(lock);
}
