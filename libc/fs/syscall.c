// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "syscall.h"
#include <fcntl.h>
#include <string.h>
#include "errno.h"
#include "fs.h"
#include "raise.h"
#include "trace.h"

#define MAX_FILES 1024

/* Offset to account for stdin=0, stdout=1, stderr=2. */
#define FD_OFFSET 3

typedef struct _handle
{
    fs_t* fs;
    fs_file_t* file;
} handle_t;

static handle_t _handles[MAX_FILES];

static size_t _assign_handle()
{
    for (size_t i = 0; i < MAX_FILES; i++)
    {
        if (_handles[i].fs == NULL && _handles[i].file == NULL)
            return i;
    }

    return (size_t)-1;
}

fs_errno_t fs_syscall_open(
    const char* pathname,
    int flags,
    uint32_t mode,
    int* ret)
{
    fs_errno_t err = 0;
    fs_t* fs = NULL;
    char suffix[FS_PATH_MAX];
    fs_file_t* file;
    size_t index;

    if (ret)
        *ret = -1;

    if (!ret)
        RAISE(FS_EINVAL);

    if (!(fs = fs_lookup(pathname, suffix)))
        RAISE(FS_ENOENT);

    if ((index = _assign_handle()) == (size_t)-1)
        RAISE(FS_EMFILE);

    CHECK(fs->fs_open(fs, suffix, flags, mode, &file));

    _handles[index].fs = fs;
    _handles[index].file = file;

    *ret = index + FD_OFFSET;

done:

    return err;
}

fs_errno_t fs_syscall_creat(
    const char* pathname,
    uint32_t mode,
    int* ret)
{
    int flags = FS_O_CREAT | FS_O_WRONLY | FS_O_TRUNC;
    return fs_syscall_open(pathname, flags, mode, ret);
}

fs_errno_t fs_syscall_close(int fd, int* ret)
{
    fs_errno_t err = 0;
    handle_t h;
    ssize_t index;

    if (ret)
        *ret = -1;

    if (!ret)
        RAISE(FS_EINVAL);

    index = fd - FD_OFFSET;

    if (index < 0 || index >= MAX_FILES)
        RAISE(FS_EBADF);

    h = _handles[index];

    if (!h.fs || !h.file)
        RAISE(FS_EINVAL);

    CHECK(h.fs->fs_close(h.file));

    memset(&_handles[index], 0, sizeof(_handles));

    *ret = 0;

done:
    return err;
}

fs_errno_t fs_syscall_readv(
    int fd,
    const fs_iovec_t* iov,
    int iovcnt,
    ssize_t* ret)
{
    fs_errno_t err = 0;
    ssize_t index;
    handle_t h;
    ssize_t nread = 0;

    if (ret)
        *ret = -1;

    if (!iov || !ret)
        RAISE(FS_EINVAL);

    index = fd - FD_OFFSET;

    if (index < 0 || index >= MAX_FILES)
        RAISE(FS_EBADF);

    h = _handles[index];

    if (!h.fs || !h.file)
        RAISE(FS_EINVAL);

    for (int i = 0; i < iovcnt; i++)
    {
        const fs_iovec_t* p = &iov[i];
        int32_t n;

        CHECK(h.fs->fs_read(h.file, p->iov_base, p->iov_len, &n));
        nread += n;

        if (n < p->iov_len)
            break;
    }

    *ret = nread;

done:
    return err;
}

fs_errno_t fs_syscall_writev(
    int fd,
    const fs_iovec_t* iov,
    int iovcnt,
    ssize_t* ret)
{
    fs_errno_t err = 0;
    int index;
    handle_t h;
    ssize_t nwritten = 0;

    if (ret)
        *ret = -1;

    if (!iov || !ret)
        RAISE(FS_EINVAL);

    index = fd - FD_OFFSET;

    if (index < 0 || index >= MAX_FILES)
        RAISE(FS_EBADF);

    h = _handles[index];

    if (!h.fs || !h.file)
        RAISE(FS_EINVAL);

    for (int i = 0; i < iovcnt; i++)
    {
        const fs_iovec_t* p = &iov[i];
        int32_t n;

        CHECK(h.fs->fs_write(h.file, p->iov_base, p->iov_len, &n));

        if (n != p->iov_len)
            RAISE(FS_EIO);

        nwritten += n;
    }

    *ret = nwritten;

done:
    return err;
}

fs_errno_t fs_syscall_stat(const char* pathname, fs_stat_t* buf, int* ret)
{
    fs_errno_t err = 0;
    fs_t* fs;
    char suffix[FS_PATH_MAX];
    fs_stat_t stat;

    if (buf)
        memset(buf, 0, sizeof(fs_stat_t));

    if (ret)
        *ret = -1;

    memset(&stat, 0, sizeof(stat));

    if (!pathname || !buf || !ret)
        RAISE(FS_EINVAL);

    if (!(fs = fs_lookup(pathname, suffix)))
        RAISE(FS_ENOENT);

    CHECK(fs->fs_stat(fs, suffix, &stat));

    *buf = stat;
    *ret = 0;

done:
    return err;
}

fs_errno_t fs_syscall_lseek(int fd, ssize_t off, int whence, ssize_t* ret)
{
    fs_errno_t err = 0;
    ssize_t index;
    handle_t h;

    if (ret)
        *ret = -1;

    if (!ret)
        RAISE(FS_EINVAL);

    index = fd - FD_OFFSET;

    if (index < 0 || index >= MAX_FILES)
        RAISE(FS_EBADF);

    h = _handles[index];

    if (!h.fs || !h.file)
        RAISE(FS_EINVAL);

    CHECK(h.fs->fs_lseek(h.file, off, whence, ret));

done:
    return err;
}

fs_errno_t fs_syscall_link(const char* oldpath, const char* newpath, int* ret)
{
    fs_errno_t err = 0;
    fs_t* old_fs;
    fs_t* new_fs;
    char old_suffix[FS_PATH_MAX];
    char new_suffix[FS_PATH_MAX];

    if (ret)
        *ret = -1;

    if (!oldpath || !newpath || !ret)
        RAISE(FS_EINVAL);

    if (!(old_fs = fs_lookup(oldpath, old_suffix)))
        RAISE(FS_ENOENT);

    if (!(new_fs = fs_lookup(newpath, new_suffix)))
        RAISE(FS_ENOENT);

    /* Disallow linking across different file systems. */
    if (old_fs != new_fs)
        RAISE(FS_ENOENT);

    CHECK(old_fs->fs_link(old_fs, old_suffix, new_suffix));

    *ret = 0;

done:
    return err;
}

fs_errno_t fs_syscall_unlink(const char* pathname, int* ret)
{
    fs_errno_t err = 0;
    fs_t* fs;
    char suffix[FS_PATH_MAX];

    if (ret)
        *ret = -1;

    if (!pathname || !ret)
        RAISE(FS_EINVAL);

    if (!(fs = fs_lookup(pathname, suffix)))
        RAISE(FS_ENOENT);

    CHECK(fs->fs_unlink(fs, suffix));

    *ret = 0;

done:
    return err;
}

fs_errno_t fs_syscall_rename(const char* oldpath, const char* newpath, int* ret)
{
    fs_errno_t err = 0;
    fs_t* old_fs;
    fs_t* new_fs;
    char old_suffix[FS_PATH_MAX];
    char new_suffix[FS_PATH_MAX];

    if (ret)
        *ret = -1;

    if (!oldpath || !newpath || !ret)
        RAISE(FS_EINVAL);

    if (!(old_fs = fs_lookup(oldpath, old_suffix)))
        RAISE(FS_ENOENT);

    if (!(new_fs = fs_lookup(newpath, new_suffix)))
        RAISE(FS_ENOENT);

    /* Disallow renaming across different file systems. */
    if (old_fs != new_fs)
        RAISE(FS_ENOENT);

    CHECK(old_fs->fs_rename(old_fs, old_suffix, new_suffix));

    *ret = 0;

done:
    return err;
}

fs_errno_t fs_syscall_truncate(const char* path, ssize_t length, int* ret)
{
    fs_errno_t err = 0;
    fs_t* fs;
    char suffix[FS_PATH_MAX];

    if (ret)
        *ret = -1;

    if (!path || !ret)
        RAISE(FS_EINVAL);

    if (!(fs = fs_lookup(path, suffix)))
        RAISE(FS_ENOENT);

    CHECK(fs->fs_truncate(fs, suffix, length));

    *ret = 0;

done:
    return err;
}

fs_errno_t fs_syscall_mkdir(const char *pathname, uint32_t mode, int* ret)
{
    fs_errno_t err = 0;
    fs_t* fs;
    char suffix[FS_PATH_MAX];

    if (ret)
        *ret = -1;

    if (!pathname || !ret)
        RAISE(FS_EINVAL);

    if (!(fs = fs_lookup(pathname, suffix)))
        RAISE(FS_ENOENT);

    CHECK(fs->fs_mkdir(fs, suffix, mode));

    *ret = 0;

done:
    return err;
}
