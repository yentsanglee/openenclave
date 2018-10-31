// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _fs_syscall_h
#define _fs_syscall_h

#include "errno.h"
#include "fs.h"

fs_errno_t fs_syscall_open(
    const char* pathname,
    int flags,
    uint32_t mode,
    int* ret);

fs_errno_t fs_syscall_close(int fd, int* ret);

fs_errno_t fs_syscall_readv(
    int fd,
    const fs_iovec_t* iov,
    int iovcnt,
    ssize_t* ret);

fs_errno_t fs_syscall_stat(const char* pathname, fs_stat_t* buf, int* ret);

#endif /* _fs_syscall_h */
