// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "gettid.h"
#include "syscall.h"

int ve_gettid(void)
{
    return (int)ve_syscall0(OE_SYS_gettid);
}
