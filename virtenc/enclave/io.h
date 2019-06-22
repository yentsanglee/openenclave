// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_IO_H
#define _VE_IO_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

#define VE_I_SENDFD (('S' << 8) | 17)
#define VE_I_RECVFD (('S' << 8) | 14)

struct ve_strrecvfd
{
    int fd;
    int uid;
    int gid;
    char __fill[8];
};

ssize_t ve_read(int fd, void* buf, size_t count);

ssize_t ve_write(int fd, const void* buf, size_t count);

int ve_close(int fd);

int ve_ioctl(int fd, unsigned long request, ...);

#endif /* _VE_IO_H */
