// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_SYSCALL_H
#define _VE_SYSCALL_H

#include <openenclave/internal/syscall/sys/syscall.h>
#include "common.h"

long ve_syscall0(long n);

long ve_syscall1(long n, long x1);

long ve_syscall2(long n, long x1, long x2);

long ve_syscall3(long n, long x1, long x2, long x3);

long ve_syscall4(long n, long x1, long x2, long x3, long x4);

long ve_syscall5(long n, long x1, long x2, long x3, long x4, long x5);

long ve_syscall6(long n, long x1, long x2, long x3, long x4, long x5, long x6);

#endif /* _VE_SYSCALL_H */
