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

fs_errno_t fs_syscall_creat(const char* pathname, uint32_t mode, int* ret);

fs_errno_t fs_syscall_close(int fd, int* ret);

fs_errno_t fs_syscall_readv(
    int fd,
    const fs_iovec_t* iov,
    int iovcnt,
    ssize_t* ret);

fs_errno_t fs_syscall_writev(
    int fd,
    const fs_iovec_t* iov,
    int iovcnt,
    ssize_t* ret);

fs_errno_t fs_syscall_stat(const char* pathname, fs_stat_t* buf, int* ret);

fs_errno_t fs_syscall_lseek(int fd, ssize_t off, int whence, ssize_t* ret);

fs_errno_t fs_syscall_link(const char* oldpath, const char* newpath, int* ret);

fs_errno_t fs_syscall_unlink(const char* pathname, int* ret);

fs_errno_t fs_syscall_rename(const char* oldpath, const char* newpath, int* ret);

fs_errno_t fs_syscall_truncate(const char* path, ssize_t length, int* ret);

fs_errno_t fs_syscall_mkdir(const char *pathname, uint32_t mode, int* ret);

fs_errno_t fs_syscall_rmdir(const char *pathname, int* ret);

#endif /* _fs_syscall_h */
