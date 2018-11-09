// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include "hostfs.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hostbatch.h"
#include "hostcalls.h"
#include "hostfs.h"
#include "raise.h"

#define HOSTFS_MAGIC 0xff646572

#define BATCH_CAPACITY 4096

#define FILE_MAGIC 0xb5c3c9db

#define DIR_MAGIC 0xa9136e28

static const fs_guid_t _GUID = FS_HOSTFS_GUID;

typedef struct _hostfs
{
    fs_t base;
    uint32_t magic;
    fs_host_batch_t* batch;
} hostfs_t;

struct _fs_file
{
    uint32_t magic;
    hostfs_t* hostfs;
    long fd;
};

struct _fs_dir
{
    uint32_t magic;
    hostfs_t* hostfs;
    void* host_dir;
    fs_dirent_t entry;
};

static bool _valid_fs(fs_t* fs)
{
    return fs != NULL && ((hostfs_t*)fs)->magic == HOSTFS_MAGIC;
}

static bool _valid_file(fs_file_t* file)
{
    return file != NULL && file->magic == FILE_MAGIC;
}

static bool _valid_dir(fs_dir_t* dir)
{
    return dir != NULL;
}

static fs_errno_t _fs_release(fs_t* fs)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;

    if (!_valid_fs(fs))
        FS_RAISE(FS_EINVAL);

    fs_host_batch_delete(hostfs->batch);

    free(fs);

done:
    return err;
}

static fs_errno_t _fs_open(
    fs_t* fs,
    const char* path,
    int flags,
    uint32_t mode,
    fs_file_t** file_out);

static fs_errno_t _fs_creat(
    fs_t* fs,
    const char* path,
    uint32_t mode,
    fs_file_t** file_out)
{
    int flags = FS_O_CREAT | FS_O_WRONLY | FS_O_TRUNC;
    return _fs_open(fs, path, flags, mode, file_out);
}

static fs_errno_t _fs_open(
    fs_t* fs,
    const char* path,
    int flags,
    uint32_t mode,
    fs_file_t** file_out)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;
    fs_file_t* file = NULL;

    if (file_out)
        *file_out = NULL;

    if (!_valid_fs(fs) || !path || !file_out)
        FS_RAISE(FS_EINVAL);

    batch = hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_OPEN;
        args->u.open.ret = -1;

        if (!(args->u.open.pathname = fs_host_batch_strdup(batch, path)))
            FS_RAISE(FS_ENOMEM);

        args->u.open.flags = flags;
        args->u.open.mode = mode;
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.open.ret < 0)
            FS_RAISE(args->err);
    }

    /* Create the file struct. */
    {
        if (!(file = calloc(1, sizeof(fs_file_t))))
            FS_RAISE(FS_ENOMEM);

        file->magic = FILE_MAGIC;
        file->hostfs = hostfs;
        file->fd = args->u.open.ret;
    }

    *file_out = file;
    file = NULL;

done:

    if (file)
        free(file);

    if (batch)
        fs_host_batch_free(batch);

    return err;
}

static fs_errno_t _fs_lseek(
    fs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out)
{
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (offset_out)
        *offset_out = 0;

    if (!_valid_file(file))
        FS_RAISE(FS_EINVAL);

    batch = file->hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_LSEEK;
        args->u.lseek.ret = -1;
        args->u.lseek.fd = file->fd;
        args->u.lseek.offset = offset;
        args->u.lseek.whence = whence;
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_ENOMEM);

        if (args->u.lseek.ret < 0)
            FS_RAISE(args->err);
    }

    *offset_out = args->u.lseek.ret;

done:
    return err;
}

static fs_errno_t _fs_read(
    fs_file_t* file,
    void* data,
    size_t size,
    ssize_t* nread)
{
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (nread)
        *nread = 0;

    /* Check parameters */
    if (!_valid_file(file) || (!data && size) || !nread)
        FS_RAISE(FS_EINVAL);

    batch = file->hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_READ;
        args->u.read.ret = -1;
        args->u.read.fd = file->fd;

        if (size)
        {
            if (!(args->u.read.buf = fs_host_batch_malloc(batch, size)))
                FS_RAISE(FS_ENOMEM);
        }

        args->u.read.count = size;
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.read.ret < 0)
            FS_RAISE(args->err);
    }

    /* Copy data onto caller's buffer. */
    if (args->u.read.ret <= size)
        memcpy(data, args->u.read.buf, args->u.read.ret);

    *nread = args->u.read.ret;

done:

    if (batch)
        fs_host_batch_free(batch);

    return err;
}

static fs_errno_t _fs_write(
    fs_file_t* file,
    const void* data,
    size_t size,
    ssize_t* nwritten)
{
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (nwritten)
        *nwritten = 0;

    /* Check parameters */
    if (!_valid_file(file) || (!data && size) || !nwritten)
        FS_RAISE(FS_EINVAL);

    batch = file->hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_WRITE;
        args->u.write.ret = -1;
        args->u.write.fd = file->fd;

        if (size)
        {
            if (!(args->u.write.buf = fs_host_batch_malloc(batch, size)))
                FS_RAISE(FS_ENOMEM);

            memcpy(args->u.write.buf, data, size);
        }

        args->u.write.count = size;
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.write.ret < 0)
            FS_RAISE(args->err);
    }

    *nwritten = args->u.write.ret;

done:

    if (batch)
        fs_host_batch_free(batch);

    return err;
}

static fs_errno_t _fs_close(fs_file_t* file)
{
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (!_valid_file(file))
        FS_RAISE(FS_EINVAL);

    batch = file->hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_CLOSE;
        args->u.close.ret = -1;
        args->u.close.fd = file->fd;
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.close.ret != 0)
            FS_RAISE(args->err);
    }

    /* Release the file struct. */
    memset(file, 0, sizeof(fs_file_t));
    free(file);

done:

    if (batch)
        fs_host_batch_free(batch);

    return err;
}

static fs_errno_t _fs_opendir(fs_t* fs, const char* path, fs_dir_t** dir_out)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;
    fs_dir_t* dir = NULL;

    if (dir_out)
        *dir_out = NULL;

    if (!_valid_fs(fs) || !path || !dir_out)
        FS_RAISE(FS_EINVAL);

    batch = hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_OPENDIR;

        if (!(args->u.opendir.name = fs_host_batch_strdup(batch, path)))
            FS_RAISE(FS_ENOMEM);
    }

    /* Create the dir struct. */
    if (!(dir = calloc(1, sizeof(fs_dir_t))))
        FS_RAISE(FS_ENOMEM);

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (!args->u.opendir.dir)
            FS_RAISE(args->err);
    }

    /* Create the dir struct. */
    dir->magic = DIR_MAGIC;
    dir->hostfs = hostfs;
    dir->host_dir = args->u.opendir.dir;
    *dir_out = dir;
    dir = NULL;

done:

    if (batch)
        fs_host_batch_free(batch);

    if (dir)
        free(dir);

    return err;
}

static fs_errno_t _fs_readdir(fs_dir_t* dir, fs_dirent_t** entry_out)
{
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;
    fs_dirent_t* entry;

    if (entry_out)
        *entry_out = NULL;

    if (!_valid_dir(dir) || !entry_out)
        FS_RAISE(FS_EINVAL);

    batch = dir->hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_READDIR;
        args->u.readdir.dir = dir->host_dir;
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (!args->u.readdir.entry)
        {
            /* Error may be zero. */
            FS_RAISE(args->err);
        }
    }

    /* Initialize the current entry. */
    entry = &dir->entry;
    entry->d_ino = args->u.readdir.buf.d_ino;
    entry->d_off = args->u.readdir.buf.d_off;
    entry->d_reclen = args->u.readdir.buf.d_reclen;
    entry->d_type = args->u.readdir.buf.d_type;
    strlcpy(entry->d_name, args->u.readdir.buf.d_name, sizeof(entry->d_name));
    *entry_out = entry;

done:

    if (batch)
        fs_host_batch_free(batch);

    return err;
}

static fs_errno_t _fs_closedir(fs_dir_t* dir)
{
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (!_valid_dir(dir))
        FS_RAISE(FS_EINVAL);

    batch = dir->hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_CLOSEDIR;
        args->u.closedir.ret = -1;
        args->u.closedir.dir = dir->host_dir;
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.closedir.ret != 0)
            FS_RAISE(args->err);
    }

    /* free the dir struct. */
    free(dir);

done:

    if (batch)
        fs_host_batch_free(batch);

    return err;
}

static fs_errno_t _fs_stat(fs_t* fs, const char* path, fs_stat_t* stat)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    batch = hostfs->batch;

    if (stat)
        memset(stat, 0, sizeof(fs_stat_t));

    if (!_valid_fs(fs) || !path || !stat)
        FS_RAISE(FS_EINVAL);

    if (strlen(path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_STAT;
        args->u.stat.ret = -1;
        strlcpy(args->u.stat.pathname, path, sizeof(args->u.stat.pathname));
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.stat.ret != 0)
            FS_RAISE(args->err);
    }

    /* Copy to user buffer. */
    stat->st_dev = args->u.stat.buf.st_dev;
    stat->st_ino = args->u.stat.buf.st_ino;
    stat->st_mode = args->u.stat.buf.st_mode;
    stat->st_nlink = args->u.stat.buf.st_nlink;
    stat->st_uid = args->u.stat.buf.st_uid;
    stat->st_gid = args->u.stat.buf.st_gid;
    stat->st_rdev = args->u.stat.buf.st_rdev;
    stat->st_size = args->u.stat.buf.st_size;
    stat->st_blksize = args->u.stat.buf.st_blksize;
    stat->st_blocks = args->u.stat.buf.st_blocks;
    stat->st_atim.tv_sec = args->u.stat.buf.st_atim.tv_sec;
    stat->st_atim.tv_nsec = args->u.stat.buf.st_atim.tv_nsec;
    stat->st_mtim.tv_sec = args->u.stat.buf.st_mtim.tv_sec;
    stat->st_mtim.tv_nsec = args->u.stat.buf.st_mtim.tv_nsec;
    stat->st_ctim.tv_sec = args->u.stat.buf.st_ctim.tv_sec;
    stat->st_ctim.tv_nsec = args->u.stat.buf.st_ctim.tv_nsec;

done:
    return err;
}

static fs_errno_t _fs_link(fs_t* fs, const char* old_path, const char* new_path)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    batch = hostfs->batch;

    if (!old_path || !new_path)
        FS_RAISE(FS_EINVAL);

    if (strlen(old_path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    if (strlen(new_path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_LINK;
        args->u.link.ret = -1;
        strlcpy(args->u.link.oldpath, old_path, sizeof(args->u.link.oldpath));
        strlcpy(args->u.link.newpath, new_path, sizeof(args->u.link.newpath));
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.link.ret != 0)
            FS_RAISE(args->err);
    }

done:

    return err;
}

static fs_errno_t _fs_unlink(fs_t* fs, const char* path)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (!_valid_fs(fs) || !path)
        FS_RAISE(FS_EINVAL);

    if (strlen(path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    batch = hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_UNLINK;
        args->u.unlink.ret = -1;
        strlcpy(args->u.unlink.path, path, sizeof(args->u.unlink.path));
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.unlink.ret != 0)
            FS_RAISE(args->err);
    }

done:

    return err;
}

static fs_errno_t _fs_rename(
    fs_t* fs,
    const char* old_path,
    const char* new_path)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    batch = hostfs->batch;

    if (!old_path || !new_path)
        FS_RAISE(FS_EINVAL);

    if (strlen(old_path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    if (strlen(new_path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_RENAME;
        args->u.rename.ret = -1;
        strlcpy(
            args->u.rename.oldpath, old_path, sizeof(args->u.rename.oldpath));
        strlcpy(
            args->u.rename.newpath, new_path, sizeof(args->u.rename.newpath));
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.rename.ret != 0)
            FS_RAISE(args->err);
    }

done:

    return err;
}

static fs_errno_t _fs_truncate(fs_t* fs, const char* path, ssize_t length)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (!_valid_fs(fs) || !path)
        FS_RAISE(FS_EINVAL);

    if (strlen(path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    batch = hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_TRUNCATE;
        args->u.truncate.ret = -1;
        args->u.truncate.length = length;
        strlcpy(args->u.truncate.path, path, sizeof(args->u.truncate.path));
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.truncate.ret != 0)
            FS_RAISE(args->err);
    }

done:

    return err;
}

static fs_errno_t _fs_mkdir(fs_t* fs, const char* path, uint32_t mode)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (!_valid_fs(fs) || !path)
        FS_RAISE(FS_EINVAL);

    if (strlen(path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    batch = hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_MKDIR;
        args->u.mkdir.ret = -1;
        args->u.mkdir.mode = mode;
        strlcpy(args->u.mkdir.pathname, path, sizeof(args->u.mkdir.pathname));
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.mkdir.ret != 0)
            FS_RAISE(args->err);
    }

done:

    return err;
}

static fs_errno_t _fs_rmdir(fs_t* fs, const char* path)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostfs_ocall_args_t args_t;
    args_t* args;

    if (!_valid_fs(fs) || !path)
        FS_RAISE(FS_EINVAL);

    if (strlen(path) >= FS_PATH_MAX)
        FS_RAISE(FS_EINVAL);

    batch = hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            FS_RAISE(FS_ENOMEM);

        args->base.guid = _GUID;
        args->op = FS_HOSTFS_RMDIR;
        args->u.rmdir.ret = -1;
        strlcpy(args->u.rmdir.pathname, path, sizeof(args->u.rmdir.pathname));
    }

    /* Perform the OCALL. */
    {
        if (fs_host_call(&_GUID, &args->base) != 0)
            FS_RAISE(FS_EIO);

        if (args->u.rmdir.ret != 0)
            FS_RAISE(args->err);
    }

done:

    return err;
}

fs_errno_t hostfs_initialize(fs_t** fs_out)
{
    fs_errno_t err = FS_EOK;
    hostfs_t* hostfs = NULL;
    fs_host_batch_t* batch = NULL;

    if (fs_out)
        *fs_out = NULL;

    if (!fs_out)
        FS_RAISE(FS_EINVAL);

    if (!(hostfs = calloc(1, sizeof(hostfs_t))))
        FS_RAISE(FS_ENOMEM);

    if (!(batch = fs_host_batch_new(BATCH_CAPACITY)))
        FS_RAISE(FS_ENOMEM);

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
    hostfs->batch = batch;

    *fs_out = &hostfs->base;
    hostfs = NULL;
    batch = NULL;

done:

    if (hostfs)
        free(hostfs);

    if (batch)
        fs_host_batch_delete(batch);

    return err;
}

int fs_hostfs_ocall(fs_hostfs_ocall_args_t* args)
{
    if (!args)
        return -1;

    if (fs_host_call(&_GUID, &args->base) != 0)
        return -1;

    return 0;
}

int fs_mount_hostfs(const char* source, const char* target)
{
    int ret = -1;
    fs_t* fs = NULL;

    /* TODO: handle source here. */

    if (!target)
        goto done;

    if (hostfs_initialize(&fs) != 0)
        goto done;

    if (fs_bind(fs, target) != 0)
        goto done;

    fs = NULL;
    ret = 0;

done:

    if (fs)
        fs->fs_release(fs);

    return ret;
}
