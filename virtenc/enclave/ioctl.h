// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_IOCTL_H
#define _VE_IOCTL_H

#include <openenclave/bits/defs.h>
#include "syscall.h"

#define VE_I_SENDFD (('S' << 8) | 17)
#define VE_I_RECVFD (('S' << 8) | 14)

struct ve_strrecvfd
{
    int fd;
    int uid;
    int gid;
    char __fill[8];
};

int ve_ioctl(int fd, unsigned long request, ...);

#endif /* _VE_IOCTL_H */
