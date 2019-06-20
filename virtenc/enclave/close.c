// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "syscall.h"

int ve_close(int fd)
{
    return ve_syscall1(OE_SYS_close, (long)fd);
}
