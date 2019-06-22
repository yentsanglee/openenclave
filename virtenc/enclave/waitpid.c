// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "waitpid.h"
#include "syscall.h"

int ve_waitpid(int pid, int* status, int options)
{
    return (int)ve_syscall4(OE_SYS_wait4, pid, (long)status, options, 0);
}
