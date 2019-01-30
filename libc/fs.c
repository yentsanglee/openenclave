// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <openenclave/internal/fs.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/uio.h>

FILE *oe_fopen_dev(int device_id, const char *path, const char *mode)
{
    oe_set_thread_default_device(device_id);
    FILE* ret = fopen(path, mode);
    oe_clear_thread_default_device();

    return ret;
}

DIR* oe_opendir_dev(int device_id, const char* pathname)
{
    oe_set_thread_default_device(device_id);
    DIR* ret = opendir(pathname);
    oe_clear_thread_default_device();

    return ret;
}

int oe_unlink_dev(int device_id, const char* pathname)
{
    oe_set_thread_default_device(device_id);
    int ret = unlink(pathname);
    oe_clear_thread_default_device();

    return ret;
}

int oe_link_dev(int device_id, const char* oldpath, const char* newpath)
{
    oe_set_thread_default_device(device_id);
    int ret = link(oldpath, newpath);
    oe_clear_thread_default_device();

    return ret;
}

int oe_rename_dev(int device_id, const char* oldpath, const char* newpath)
{
    oe_set_thread_default_device(device_id);
    int ret = rename(oldpath, newpath);
    oe_clear_thread_default_device();

    return ret;
}

int oe_mkdir_dev(int device_id, const char* pathname, oe_mode_t mode)
{
    oe_set_thread_default_device(device_id);
    int ret = mkdir(pathname, mode);
    oe_clear_thread_default_device();

    return ret;
}

int oe_rmdir_dev(int device_id, const char* pathname)
{
    oe_set_thread_default_device(device_id);
    int ret = rmdir(pathname);
    oe_clear_thread_default_device();

    return ret;
}

int oe_stat_dev(int device_id, const char* pathname, struct oe_stat* buf)
{
    oe_set_thread_default_device(device_id);
    int ret = stat(pathname, (struct stat*)buf);
    oe_clear_thread_default_device();

    return ret;
}

int oe_truncate_dev(int device_id, const char* path, oe_off_t length)
{
    oe_set_thread_default_device(device_id);
    int ret = truncate(path, length);
    oe_clear_thread_default_device();

    return ret;
}

#if 0
/* ATTN: include these! */
ssize_t oe_readv_dev(int device_id, int fd, const oe_iovec_t* iov, int iovcnt)
{
    oe_set_thread_default_device(device_id);
    ssize_t ret = readv(fd, (const struct iovec*)iov, iovcnt);
    oe_clear_thread_default_device();

    return ret;
}

ssize_t oe_writev_dev(int device_id, int fd, const oe_iovec_t* iov, int iovcnt)
{
    oe_set_thread_default_device(device_id);
    ssize_t ret = writev(fd, (const struct iovec*)iov, iovcnt);
    oe_clear_thread_default_device();

    return ret;
}
#endif

int oe_access_dev(int device_id, const char *pathname, int mode)
{
    oe_set_thread_default_device(device_id);
    int ret = access(pathname, mode);
    oe_clear_thread_default_device();

    return ret;
}
