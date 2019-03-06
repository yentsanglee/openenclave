// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <string.h>
#include "syscallx.h"

void _start(void)
{
#if 1
    syscall3(SYS_write, 1, (long)"xxx\n", 4);
    syscall3(SYS_write, 1, (long)"xxx\n", 4);
    syscall3(SYS_write, 1, (long)"yyy\n", 4);
    syscall3(SYS_write, 1, (long)"zzz\n", 4);
#else
    syscall6(99, 10, 20, 30, 40, 50, 60);
#endif
}
