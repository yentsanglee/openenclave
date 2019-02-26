// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/dirent.h>
#include <openenclave/corelibc/errno.h>
#include <openenclave/corelibc/fcntl.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/corelibc/sys/stat.h>
#include <openenclave/corelibc/sys/syscall.h>
#include <openenclave/corelibc/sys/uio.h>
#include <openenclave/corelibc/unistd.h>
#include <openenclave/internal/device.h>

typedef int (*ioctl_proc)(
    int fd,
    unsigned long request,
    long arg1,
    long arg2,
    long arg3,
    long arg4);

int __oe_syscall(
    long num,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long arg5,
    long arg6,
    long* ret_out)
{
    if (ret_out)
        *ret_out = 0;

    oe_errno = 0;

    if (!ret_out)
    {
        oe_errno = EINVAL;
        return -1;
    }

    /* Handle the software system call. */
    switch (num)
    {
        case OE_SYS_creat:
        {
            const char* pathname = (const char*)arg1;
            mode_t mode = (mode_t)arg2;
            int flags = (OE_O_CREAT | OE_O_WRONLY | OE_O_TRUNC);

            *ret_out = oe_open(pathname, flags, mode);

            if (oe_errno == ENOENT)
            {
                /* Not handled. Let caller dispatch this syscall. */
                return -1;
            }

            return 0;
        }
        case OE_SYS_open:
        {
            const char* pathname = (const char*)arg1;
            int flags = (int)arg2;
            uint32_t mode = (uint32_t)arg3;

            *ret_out = oe_open(pathname, flags, mode);

            if (oe_errno == ENOENT)
                return -1;

            return 0;
        }
        case OE_SYS_lseek:
        {
            int fd = (int)arg1;
            ssize_t off = (ssize_t)arg2;
            int whence = (int)arg3;

            *ret_out = oe_lseek(fd, off, whence);

            return 0;
        }
        case OE_SYS_readv:
        {
            int fd = (int)arg1;
            const struct oe_iovec* iov = (const struct oe_iovec*)arg2;
            int iovcnt = (int)arg3;

            *ret_out = oe_readv(fd, iov, iovcnt);

            return 0;
        }
        case OE_SYS_writev:
        {
            int fd = (int)arg1;
            const struct oe_iovec* iov = (const struct oe_iovec*)arg2;
            int iovcnt = (int)arg3;

            *ret_out = oe_writev(fd, iov, iovcnt);

            return 0;
        }
        case OE_SYS_read:
        {
            int fd = (int)arg1;
            void* buf = (void*)arg2;
            size_t count = (size_t)arg3;

            *ret_out = oe_read(fd, buf, count);

            return 0;
        }
        case OE_SYS_write:
        {
            int fd = (int)arg1;
            const void* buf = (void*)arg2;
            size_t count = (size_t)arg3;

            *ret_out = oe_write(fd, buf, count);

            return 0;
        }
        case OE_SYS_close:
        {
            int fd = (int)arg1;

            *ret_out = oe_close(fd);

            return 0;
        }
        case OE_SYS_stat:
        {
            const char* pathname = (const char*)arg1;
            struct oe_stat* buf_out = (struct oe_stat*)arg2;
            struct oe_stat buf;

            *ret_out = oe_stat(pathname, &buf);

            memcpy(buf_out, &buf, sizeof(struct oe_stat));

            return 0;
        }
        case OE_SYS_link:
        {
            const char* oldpath = (const char*)arg1;
            const char* newpath = (const char*)arg2;

            *ret_out = oe_link(oldpath, newpath);

            return 0;
        }
        case OE_SYS_unlink:
        {
            const char* pathname = (const char*)arg1;

            *ret_out = oe_unlink(pathname);

            return 0;
        }
        case OE_SYS_rename:
        {
            const char* oldpath = (const char*)arg1;
            const char* newpath = (const char*)arg2;

            *ret_out = oe_rename(oldpath, newpath);

            return 0;
        }
        case OE_SYS_truncate:
        {
            const char* path = (const char*)arg1;
            ssize_t length = (ssize_t)arg2;

            *ret_out = oe_truncate(path, length);

            return 0;
        }
        case OE_SYS_mkdir:
        {
            const char* pathname = (const char*)arg1;
            uint32_t mode = (uint32_t)arg2;

            *ret_out = oe_mkdir(pathname, mode);

            return 0;
        }
        case OE_SYS_rmdir:
        {
            const char* pathname = (const char*)arg1;
            *ret_out = oe_rmdir(pathname);
            return 0;
        }
        case OE_SYS_access:
        {
            const char* pathname = (const char*)arg1;
            int mode = (int)arg2;

            *ret_out = oe_access(pathname, mode);

            return 0;
        }
        case OE_SYS_getdents:
        case OE_SYS_getdents64:
        {
            unsigned int fd = (unsigned int)arg1;
            struct oe_dirent* ent = (struct oe_dirent*)arg2;
            unsigned int count = (unsigned int)arg3;
            *ret_out = oe_getdents(fd, ent, count);

            return 0;
        }
        case OE_SYS_ioctl:
        {
            int fd = (int)arg1;
            unsigned long request = (unsigned long)arg2;
            long p1 = arg3;
            long p2 = arg4;
            long p3 = arg5;
            long p4 = arg6;

            *ret_out = ((ioctl_proc)oe_ioctl)(fd, request, p1, p2, p3, p4);

            return 0;
        }
        case OE_SYS_fcntl:
        {
            /* Silently ignore. */
            return 0;
        }
        case OE_SYS_mount:
        {
            const char* source = (const char*)arg1;
            const char* target = (const char*)arg2;
            const char* fstype = (const char*)arg3;
            unsigned long flags = (unsigned long)arg4;
            void* data = (void*)arg5;

            *ret_out = oe_mount(source, target, fstype, flags, data);

            return 0;
        }
        case OE_SYS_umount2:
        {
            const char* target = (const char*)arg1;
            int flags = (int)arg2;

            (void)flags;

            *ret_out = oe_umount(target);

            return 0;
        }
        case OE_SYS_getcwd:
        {
            char* buf = (char*)arg1;
            size_t size = (size_t)arg2;

            if (!oe_getcwd(buf, size))
            {
                *ret_out = -1;
            }
            else
            {
                *ret_out = (long)size;
            }

            return 0;
        }
        case OE_SYS_chdir:
        {
            char* path = (char*)arg1;

            *ret_out = oe_chdir(path);

            return 0;
        }
        default:
        {
            oe_errno = EINVAL;
            return -1;
        }
    }

    /* Unreachable */
    return -1;
}
