// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "syscall.h"
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "errno.h"
#include "fs.h"

int fs_handle_syscall(
    long num,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long arg5,
    long arg6,
    long* ret_out,
    fs_errno_t* err_out)
{
    if (ret_out)
        *ret_out = 0;

    if (err_out)
        *err_out = 0;

    if (!ret_out || !err_out)
        return -1;

    /* Ignore file descriptor operations on stdin, stdout, and stderr. */
    switch (num)
    {
        case SYS_lseek:
        case SYS_readv:
        case SYS_writev:
        case SYS_read:
        case SYS_write:
        case SYS_close:
        {
            int fd = (int)arg1;

            switch (fd)
            {
                case STDIN_FILENO:
                case STDOUT_FILENO:
                case STDERR_FILENO:
                    return -1;
            }
            break;
        }
    }

    /* Handle the software system call. */
    switch (num)
    {
        case SYS_creat:
        {
            const char* pathname = (const char*)arg1;
            uint32_t mode = (uint32_t)arg2;
            int ret;
            *err_out = fs_creat(pathname, mode, &ret);

            if (*err_out == FS_ENOENT)
                return -1;

            *ret_out = ret;
            return 0;
        }
        case SYS_open:
        {
            const char* pathname = (const char*)arg1;
            int flags = (int)arg2;
            uint32_t mode = (uint32_t)arg3;
            int ret = -1;
            *err_out = fs_open(pathname, flags, mode, &ret);

            if (*err_out == FS_ENOENT)
                return -1;

            *ret_out = ret;
            return 0;
        }
        case SYS_lseek:
        {
            int fd = (int)arg1;
            ssize_t off = (ssize_t)arg2;
            int whence = (int)arg3;
            ssize_t ret = -1;
            *err_out = fs_lseek(fd, off, whence, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_readv:
        {
            int fd = (int)arg1;
            const fs_iovec_t* iov = (const fs_iovec_t*)arg2;
            int iovcnt = (int)arg3;
            ssize_t ret = -1;
            *err_out = fs_readv(fd, iov, iovcnt, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_writev:
        {
            int fd = (int)arg1;
            const fs_iovec_t* iov = (const fs_iovec_t*)arg2;
            int iovcnt = (int)arg3;
            ssize_t ret = -1;
            *err_out = fs_writev(fd, iov, iovcnt, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_read:
        {
            fs_iovec_t iov;
            int fd = (int)arg1;
            iov.iov_base = (void*)arg2;
            iov.iov_len = (size_t)arg3;
            ssize_t ret = -1;
            *err_out = fs_readv(fd, &iov, 1, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_write:
        {
            fs_iovec_t iov;
            int fd = (int)arg1;
            iov.iov_base = (void*)arg2;
            iov.iov_len = (size_t)arg3;
            ssize_t ret = -1;
            *err_out = fs_writev(fd, &iov, 1, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_close:
        {
            int fd = (int)arg1;
            int ret = -1;
            *err_out = fs_close(fd, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_stat:
        {
            const char* pathname = (const char*)arg1;
            struct stat* buf_out = (struct stat*)arg2;
            fs_stat_t buf;
            int ret = -1;
            *err_out = fs_stat(pathname, &buf, &ret);
            *ret_out = ret;
            buf_out->st_dev = buf.st_dev;
            buf_out->st_ino = buf.st_ino;
            buf_out->st_mode = buf.st_mode;
            buf_out->st_nlink = buf.st_nlink;
            buf_out->st_uid = buf.st_uid;
            buf_out->st_gid = buf.st_gid;
            buf_out->st_rdev = buf.st_rdev;
            buf_out->st_size = buf.st_size;
            buf_out->st_blksize = buf.st_blksize;
            buf_out->st_blocks = buf.st_blocks;
            buf_out->st_atim.tv_sec = buf.st_atim.tv_sec;
            buf_out->st_atim.tv_nsec = buf.st_atim.tv_nsec;
            buf_out->st_mtim.tv_sec = buf.st_mtim.tv_sec;
            buf_out->st_mtim.tv_nsec = buf.st_mtim.tv_nsec;
            buf_out->st_ctim.tv_sec = buf.st_ctim.tv_sec;
            buf_out->st_ctim.tv_nsec = buf.st_ctim.tv_nsec;
            return 0;
        }
        case SYS_link:
        {
            const char* oldpath = (const char*)arg1;
            const char* newpath = (const char*)arg2;
            int ret = -1;
            *err_out = fs_link(oldpath, newpath, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_unlink:
        {
            const char* pathname = (const char*)arg1;
            int ret = -1;
            *err_out = fs_unlink(pathname, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_rename:
        {
            const char* oldpath = (const char*)arg1;
            const char* newpath = (const char*)arg2;
            int ret = -1;
            *err_out = fs_rename(oldpath, newpath, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_truncate:
        {
            const char* path = (const char*)arg1;
            ssize_t length = (ssize_t)arg2;
            int ret = -1;
            *err_out = fs_truncate(path, length, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_mkdir:
        {
            const char* pathname = (const char*)arg1;
            uint32_t mode = (uint32_t)arg2;
            int ret = -1;
            *err_out = fs_mkdir(pathname, mode, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_rmdir:
        {
            const char* pathname = (const char*)arg1;
            int ret = -1;
            *err_out = fs_rmdir(pathname, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_access:
        {
            const char* pathname = (const char*)arg1;
            int mode = (int)arg2;
            int ret = -1;
            *err_out = fs_access(pathname, mode, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_getcwd:
        {
            char* buf = (char*)arg1;
            size_t size = (size_t)arg2;
            int ret;
            *err_out = fs_getcwd(buf, size, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_chdir:
        {
            const char* path = (const char*)arg1;
            int ret = -1;
            *err_out = fs_chdir(path, &ret);
            *ret_out = ret;
            return 0;
        }
        case SYS_ioctl:
        {
            /* Silently ignore. */
            return 0;
        }
        case SYS_fcntl:
        {
            /* Silently ignore. */
            return 0;
        }
        default:
        {
            return -1;
        }
    }

    /* Unreachable */
    return -1;
}
