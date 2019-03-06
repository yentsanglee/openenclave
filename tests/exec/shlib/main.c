// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <string.h>
#include "syscallx.h"

void _start(void)
{
#if 1
    int fd = 1;
    const void* buf = "hello\n";
    size_t count = strlen(buf);
    syscall3(SYS_write, (long)fd, (long)buf, (long)count);
#else
    syscall6(99, 10, 20, 30, 40, 50, 60);
#endif
}
