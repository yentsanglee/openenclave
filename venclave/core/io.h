// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_IO_H
#define _VE_ENCLAVE_IO_H

#include "common.h"

#define VE_STDIN_FILENO 0
#define VE_STDOUT_FILENO 1
#define VE_STDERR_FILENO 2

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

int ve_readn(int fd, void* buf, size_t count);

int ve_writen(int fd, const void* buf, size_t count);

int ve_close(int fd);

int ve_ioctl(int fd, unsigned long request, ...);

int ve_pipe(int pipefd[2]);

int ve_dup(int fd);

int ve_getdtablesize(void);

#endif /* _VE_ENCLAVE_IO_H */
