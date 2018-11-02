// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "hostfs.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "raise.h"

#define HOSTFS_MAGIC 0xff646572

typedef struct _hostfs
{
    fs_t base;

    /* Should contain the value of the HOSTFS_MAGIC macro. */
    uint32_t magic;

} hostfs_t;

static bool _valid_fs(fs_t* fs)
{
    return fs != NULL;
}

static bool _valid_file(fs_file_t* file)
{
    return file != NULL;
}

static bool _valid_dir(fs_dir_t* dir)
{
    return dir != NULL;
}

static fs_errno_t _fs_read(
    fs_file_t* file,
    void* data,
    uint32_t size,
    int32_t* nread)
{
    fs_errno_t err = FS_EOK;

    if (nread)
        *nread = 0;

    if (!_valid_file(file) || !data || !nread)
        RAISE(FS_EINVAL);

done:
    return err;
}

static fs_errno_t _fs_write(
    fs_file_t* file,
    const void* data,
    uint32_t size,
    int32_t* nwritten)
{
    fs_errno_t err = FS_EOK;

    if (nwritten)
        *nwritten = 0;

    /* Check parameters */
    if (!_valid_file(file) || (!data && size) || !nwritten)
        RAISE(FS_EINVAL);

done:
    return err;
}

static fs_errno_t _fs_close(fs_file_t* file)
{
    fs_errno_t err = FS_EOK;

    if (!_valid_file(file))
        RAISE(FS_EINVAL);

done:
    return err;
}

static fs_errno_t _fs_release(fs_t* fs)
{
    fs_errno_t err = FS_EOK;

    if (!_valid_fs(fs))
        RAISE(FS_EINVAL);

done:
    return err;
}

static fs_errno_t _fs_opendir(fs_t* fs, const char* path, fs_dir_t** dir)
{
    fs_errno_t err = FS_EOK;

    if (dir)
        *dir = NULL;

    if (!_valid_fs(fs) || !path || !dir)
        RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_readdir(fs_dir_t* dir, fs_dirent_t** ent)
{
    fs_errno_t err = FS_EOK;

    if (ent)
        *ent = NULL;

    if (!_valid_dir(dir))
        RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_closedir(fs_dir_t* dir)
{
    fs_errno_t err = FS_EOK;

    if (!_valid_dir(dir))
        RAISE(FS_EINVAL);

done:
    return err;
}

static fs_errno_t _fs_open(
    fs_t* fs,
    const char* path,
    int flags,
    uint32_t mode,
    fs_file_t** file_out)
{
    fs_errno_t err = FS_EOK;

    if (file_out)
        *file_out = NULL;

    if (!_valid_fs(fs) || !path || !file_out)
        RAISE(FS_EINVAL);
done:

    return err;
}

static fs_errno_t _fs_mkdir(fs_t* fs, const char* path, uint32_t mode)
{
    fs_errno_t err = FS_EOK;

    if (!_valid_fs(fs) || !path)
        RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_creat(
    fs_t* fs,
    const char* path,
    uint32_t mode,
    fs_file_t** file_out)
{
    int flags = FS_O_CREAT | FS_O_WRONLY | FS_O_TRUNC;
    return _fs_open(fs, path, flags, mode, file_out);
}

static fs_errno_t _fs_link(fs_t* fs, const char* old_path, const char* new_path)
{
    fs_errno_t err = FS_EOK;

    if (!old_path || !new_path)
        RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_rename(
    fs_t* fs,
    const char* old_path,
    const char* new_path)
{
    fs_errno_t err = FS_EOK;

    if (!_valid_fs(fs) || !old_path || !new_path)
        RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_unlink(fs_t* fs, const char* path)
{
    fs_errno_t err = FS_EOK;

    if (!_valid_fs(fs) || !path)
        RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_truncate(fs_t* fs, const char* path, ssize_t length)
{
    fs_errno_t err = FS_EOK;

    if (!_valid_fs(fs) || !path)
        RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_rmdir(fs_t* fs, const char* path)
{
    fs_errno_t err = FS_EOK;

    if (!_valid_fs(fs) || !path)
        RAISE(FS_EINVAL);

done:

    return err;
}

static fs_errno_t _fs_stat(fs_t* fs, const char* path, fs_stat_t* stat)
{
    fs_errno_t err = FS_EOK;

    if (stat)
        memset(stat, 0, sizeof(fs_stat_t));

    if (!_valid_fs(fs) || !path || !stat)
        RAISE(FS_EINVAL);

done:
    return err;
}

static fs_errno_t _fs_lseek(
    fs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out)
{
    fs_errno_t err = FS_EOK;

    if (offset_out)
        *offset_out = 0;

    if (!_valid_file(file))
        RAISE(FS_EINVAL);

done:
    return err;
}

fs_errno_t hostfs_initialize(fs_t** fs_out)
{
    fs_errno_t err = FS_EOK;
    hostfs_t* hostfs = NULL;

    if (fs_out)
        *fs_out = NULL;

    if (!fs_out)
        RAISE(FS_EINVAL);

    if (!(hostfs = calloc(1, sizeof(hostfs_t))))
        RAISE(FS_ENOMEM);

    hostfs->base.fs_release = _fs_release;
    hostfs->base.fs_creat = _fs_creat;
    hostfs->base.fs_open = _fs_open;
    hostfs->base.fs_lseek = _fs_lseek;
    hostfs->base.fs_read = _fs_read;
    hostfs->base.fs_write = _fs_write;
    hostfs->base.fs_close = _fs_close;
    hostfs->base.fs_opendir = _fs_opendir;
    hostfs->base.fs_readdir = _fs_readdir;
    hostfs->base.fs_closedir = _fs_closedir;
    hostfs->base.fs_stat = _fs_stat;
    hostfs->base.fs_link = _fs_link;
    hostfs->base.fs_unlink = _fs_unlink;
    hostfs->base.fs_rename = _fs_rename;
    hostfs->base.fs_truncate = _fs_truncate;
    hostfs->base.fs_mkdir = _fs_mkdir;
    hostfs->base.fs_rmdir = _fs_rmdir;

    hostfs->magic = HOSTFS_MAGIC;

    *fs_out = &hostfs->base;
    hostfs = NULL;

done:

    if (hostfs)
        free(hostfs);

    return err;
}
