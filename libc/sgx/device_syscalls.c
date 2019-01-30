// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/device.h>

int oe_handle_device_syscall(
    long num,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long arg5,
    long arg6,
    long* ret_out,
    int* errno_out)
{
    if (ret_out)
        *ret_out = 0;

    if (errno_out)
        *errno_out = 0;

    if (!ret_out || !errno_out)
        return -1;

    oe_errno = 0;

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
            oe_mode_t mode = (oe_mode_t)arg2;

            *ret_out = oe_open(pathname, O_CREAT|O_WRONLY|O_TRUNC, mode);
            *errno_out = oe_errno;

            if (*errno_out == OE_ENOENT)
            {
                /* Not handled. Let caller dispatch this syscall. */
                return -1;
            }

            return 0;
        }
        case SYS_open:
        {
            const char* pathname = (const char*)arg1;
            int flags = (int)arg2;
            uint32_t mode = (uint32_t)arg3;

            *ret_out = oe_open(pathname, flags, mode);
            *errno_out = oe_errno;

            if (*errno_out == OE_ENOENT)
                return -1;

            return 0;
        }
        case SYS_lseek:
        {
            int fd = (int)arg1;
            ssize_t off = (ssize_t)arg2;
            int whence = (int)arg3;

            *ret_out = oe_lseek(fd, off, whence);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_readv:
        {
            int fd = (int)arg1;
            const oe_iovec_t* iov = (const oe_iovec_t*)arg2;
            int iovcnt = (int)arg3;

            *ret_out = oe_readv(fd, iov, iovcnt);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_writev:
        {
            int fd = (int)arg1;
            const oe_iovec_t* iov = (const oe_iovec_t*)arg2;
            int iovcnt = (int)arg3;

            *ret_out = oe_writev(fd, iov, iovcnt);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_read:
        {
            int fd = (int)arg1;
            void* buf = (void*)arg2;
            size_t count = (size_t)arg3;

            *ret_out = oe_read(fd, buf, count);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_write:
        {
            int fd = (int)arg1;
            const void* buf = (void*)arg2;
            size_t count = (size_t)arg3;

            *ret_out = oe_write(fd, buf, count);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_close:
        {
            int fd = (int)arg1;

            *ret_out = oe_close(fd);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_stat:
        {
            const char* pathname = (const char*)arg1;
            struct stat* buf_out = (struct stat*)arg2;
            struct oe_stat buf;

            *ret_out = oe_stat(pathname, &buf);
            *errno_out = oe_errno;

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

            *ret_out = oe_link(oldpath, newpath);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_unlink:
        {
            const char* pathname = (const char*)arg1;

            *ret_out = oe_unlink(pathname);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_rename:
        {
            const char* oldpath = (const char*)arg1;
            const char* newpath = (const char*)arg2;

            *ret_out = oe_rename(oldpath, newpath);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_truncate:
        {
            const char* path = (const char*)arg1;
            ssize_t length = (ssize_t)arg2;

            *ret_out = oe_truncate(path, length);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_mkdir:
        {
            const char* pathname = (const char*)arg1;
            uint32_t mode = (uint32_t)arg2;

            *ret_out = oe_mkdir(pathname, mode);
            *errno_out = oe_errno;

            return 0;
        }
        case SYS_rmdir:
        {
            const char* pathname = (const char*)arg1;
            int ret = -1;
            *errno_out = oe_rmdir(pathname);
            *ret_out = ret;
            return 0;
        }
        case SYS_access:
        {
            const char* pathname = (const char*)arg1;
            int mode = (int)arg2;

            *ret_out = oe_access(pathname, mode);
            *errno_out = oe_errno;

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
