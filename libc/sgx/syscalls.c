// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define __OE_NEED_TIME_CALLS
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/device.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/syscall.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

static oe_syscall_hook_t _hook;
static oe_spinlock_t _lock;

static const uint64_t _SEC_TO_MSEC = 1000UL;
static const uint64_t _MSEC_TO_USEC = 1000UL;
static const uint64_t _MSEC_TO_NSEC = 1000000UL;

typedef int (*ioctl_proc)(
    int fd,
    unsigned long request,
    long arg1,
    long arg2,
    long arg3,
    long arg4);

static int _handle_device_syscall(
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

    /* Handle the software system call. */
    switch (num)
    {
        case SYS_creat:
        {
            const char* pathname = (const char*)arg1;
            mode_t mode = (mode_t)arg2;

            *ret_out = oe_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
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
            *ret_out = oe_rmdir(pathname);
            *errno_out = oe_errno;
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
            int fd = (int)arg1;
            unsigned long request = (unsigned long)arg2;
            long p1 = arg3;
            long p2 = arg4;
            long p3 = arg5;
            long p4 = arg6;

            *ret_out = ((ioctl_proc)oe_ioctl)(fd, request, p1, p2, p3, p4);
            *errno_out = oe_errno;

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
static long _syscall_mmap(long n, ...)
{
    /* Always fail */
    OE_UNUSED(n);
    return EPERM;
}

static long _syscall_clock_gettime(long n, long x1, long x2)
{
    clockid_t clk_id = (clockid_t)x1;
    struct timespec* tp = (struct timespec*)x2;
    int ret = -1;
    uint64_t msec;

    OE_UNUSED(n);

    if (!tp)
        goto done;

    if (clk_id != CLOCK_REALTIME)
    {
        /* Only supporting CLOCK_REALTIME */
        oe_assert("clock_gettime(): panic" == NULL);
        goto done;
    }

    if ((msec = oe_get_time()) == (uint64_t)-1)
        goto done;

    tp->tv_sec = msec / _SEC_TO_MSEC;
    tp->tv_nsec = (msec % _SEC_TO_MSEC) * _MSEC_TO_NSEC;

    ret = 0;

done:

    return ret;
}

static long _syscall_gettimeofday(long n, long x1, long x2)
{
    struct timeval* tv = (struct timeval*)x1;
    void* tz = (void*)x2;
    int ret = -1;
    uint64_t msec;

    OE_UNUSED(n);

    if (tv)
        oe_memset(tv, 0, sizeof(struct timeval));

    if (tz)
        oe_memset(tz, 0, sizeof(struct timezone));

    if (!tv)
        goto done;

    if ((msec = oe_get_time()) == (uint64_t)-1)
        goto done;

    tv->tv_sec = msec / _SEC_TO_MSEC;
    tv->tv_usec = msec % _MSEC_TO_USEC;

    ret = 0;

done:
    return ret;
}

static long _syscall_nanosleep(long n, long x1, long x2)
{
    const struct timespec* req = (struct timespec*)x1;
    struct timespec* rem = (struct timespec*)x2;
    size_t ret = -1;
    uint64_t milliseconds = 0;

    OE_UNUSED(n);

    if (rem)
        oe_memset(rem, 0, sizeof(*rem));

    if (!req)
        goto done;

    /* Convert timespec to milliseconds */
    milliseconds += req->tv_sec * 1000UL;
    milliseconds += req->tv_nsec / 1000000UL;

    /* Perform OCALL */
    ret = oe_sleep(milliseconds);

done:

    return ret;
}

/* Intercept __syscalls() from MUSL */
long __syscall(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    oe_spin_lock(&_lock);
    oe_syscall_hook_t hook = _hook;
    oe_spin_unlock(&_lock);

    /* Invoke the syscall hook if any */
    if (hook)
    {
        long ret = -1;

        if (hook(n, x1, x2, x3, x4, x5, x6, &ret) == OE_OK)
        {
            /* The hook handled the syscall */
            return ret;
        }

        /* The hook ignored the syscall so fall through */
    }

    /* Handle any file-system syscalls. */
    {
        long ret;
        int err;

        if (_handle_device_syscall(n, x1, x2, x3, x4, x5, x6, &ret, &err) == 0)
        {
            errno = err;
            return ret;
        }
    }

    switch (n)
    {
        case SYS_nanosleep:
            return _syscall_nanosleep(n, x1, x2);
        case SYS_gettimeofday:
            return _syscall_gettimeofday(n, x1, x2);
        case SYS_clock_gettime:
            return _syscall_clock_gettime(n, x1, x2);
        case SYS_mmap:
            return _syscall_mmap(n, x1, x2, x3, x4, x5, x6);
        default:
        {
            /* All other MUSL-initiated syscalls are aborted. */
            fprintf(stderr, "error: __syscall(): n=%lu\n", n);
            abort();
            return 0;
        }
    }

    return 0;
}

/* Intercept __syscalls_cp() from MUSL */
long __syscall_cp(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    return __syscall(n, x1, x2, x3, x4, x5, x6);
}

long syscall(long number, ...)
{
    va_list ap;

    va_start(ap, number);
    long x1 = va_arg(ap, long);
    long x2 = va_arg(ap, long);
    long x3 = va_arg(ap, long);
    long x4 = va_arg(ap, long);
    long x5 = va_arg(ap, long);
    long x6 = va_arg(ap, long);
    long ret = __syscall(number, x1, x2, x3, x4, x5, x6);
    va_end(ap);

    return ret;
}

void oe_register_syscall_hook(oe_syscall_hook_t hook)
{
    oe_spin_lock(&_lock);
    _hook = hook;
    oe_spin_unlock(&_lock);
}
