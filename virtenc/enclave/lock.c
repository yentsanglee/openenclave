// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "lock.h"
#include "futex.h"
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

#define __lock __ve_lock
#define __unlock __ve_unlock
#define __futexwait ve_futex_wait
#define __wake ve_futex_wake

#include "../../3rdparty/musl/musl/src/thread/__lock.c"

void ve_lock(ve_lock_t* lock)
{
    return __ve_lock(lock);
}

void ve_unlock(ve_lock_t* lock)
{
    return __ve_unlock(lock);
}
