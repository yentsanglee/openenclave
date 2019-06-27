// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "syscall.h"
#include "time.h"

int ve_nanosleep(const struct ve_timespec* req, struct ve_timespec* rem)
{
    return (int)ve_syscall2(VE_SYS_nanosleep, (long)req, (long)rem);
}
