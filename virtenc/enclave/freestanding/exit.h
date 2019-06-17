// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FREESTANDING_EXIT_H
#define _FREESTANDING_EXIT_H

#include "../freestanding/defs.h"
#include "../freestanding/syscall.h"

FS_INLINE void fs_exit(int status)
{
    fs_syscall1(FS_SYS_exit, status);
}

#endif /* _FREESTANDING_EXIT_H */
