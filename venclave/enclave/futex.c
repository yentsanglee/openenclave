// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "futex.h"
#include "syscall.h"

static const int FUTEX_WAIT = 0;
static const int FUTEX_WAKE = 1;
static const int FUTEX_PRIVATE_FLAG = 128;

int ve_futex_wait(volatile int* uaddr, int val, int priv)
{
    int op = FUTEX_WAIT;

    if (priv)
        op |= FUTEX_PRIVATE_FLAG;

    return (int)ve_syscall6(VE_SYS_futex, (long)uaddr, op, val, 0, 0, 0);
}

int ve_futex_wake(volatile int* uaddr, int val, int priv)
{
    int op = FUTEX_WAKE;

    if (priv)
        op |= FUTEX_PRIVATE_FLAG;

    return (int)ve_syscall6(VE_SYS_futex, (long)uaddr, op, val, 0, 0, 0);
}
