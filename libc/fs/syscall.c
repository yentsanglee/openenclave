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

typedef struct _file_entry
{
    fs_t* fs;
    fs_file_t* file;
} file_entry_t;

static file_entry_t _file_entries[MAX_FILES];

static size_t _assign_file_entry()
{
    for (size_t i = 0; i < MAX_FILES; i++)
    {
        if (_file_entries[i].fs == NULL && _file_entries[i].file == NULL)
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

    if ((index = _assign_file_entry()) == (size_t)-1)
        RAISE(FS_EMFILE);

    CHECK(fs->fs_open(fs, suffix, flags, mode, &file));

    _file_entries[index].fs = fs;
    _file_entries[index].file = file;

    *ret = index + FD_OFFSET;

done:

    return err;
}

fs_errno_t fs_syscall_close(int fd, int* ret)
{
    fs_errno_t err = 0;
    file_entry_t* entry;
    int index;

    if (ret)
        *ret = -1;

    if (!ret)
        RAISE(FS_EINVAL);

    index = fd - FD_OFFSET;

    if (index < 0 || index >= MAX_FILES)
        RAISE(FS_EBADF);

    entry = &_file_entries[index];

    if (!entry->fs || !entry->file)
        RAISE(FS_EINVAL);

    CHECK(entry->fs->fs_close(entry->file));

    memset(&_file_entries[index], 0, sizeof(_file_entries));

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
    int index;
    fs_t* fs;
    fs_file_t* file;
    ssize_t nread = 0;

    if (ret)
        *ret = -1;

    if (!iov || !ret)
        RAISE(FS_EINVAL);

    index = fd - FD_OFFSET;

    if (index < 0 || index >= MAX_FILES)
        RAISE(FS_EBADF);

    fs = _file_entries[index].fs;
    file = _file_entries[index].file;

    if (!fs || !file)
        RAISE(FS_EINVAL);

    for (int i = 0; i < iovcnt; i++)
    {
        const fs_iovec_t* p = &iov[i];
        int32_t n;

        CHECK(fs->fs_read(file, p->iov_base, p->iov_len, &n));
        nread += n;

        if (n < iov->iov_len)
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
    fs_t* fs;
    fs_file_t* file;
    ssize_t nwritten = 0;

    if (ret)
        *ret = -1;

    if (!iov || !ret)
        RAISE(FS_EINVAL);

    index = fd - FD_OFFSET;

    if (index < 0 || index >= MAX_FILES)
        RAISE(FS_EBADF);

    fs = _file_entries[index].fs;
    file = _file_entries[index].file;

    if (!fs || !file)
        RAISE(FS_EINVAL);

    for (int i = 0; i < iovcnt; i++)
    {
        const fs_iovec_t* p = &iov[i];
        int32_t n;

        CHECK(fs->fs_write(file, p->iov_base, p->iov_len, &n));

        if (n != iov->iov_len)
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
    char suffix[FS_PATH_MAX];
    fs_t* fs;
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
