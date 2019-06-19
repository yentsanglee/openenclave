// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "getpid.h"
#include "syscall.h"

int ve_getpid(void)
{
    return (int)ve_syscall(OE_SYS_getpid);
}
