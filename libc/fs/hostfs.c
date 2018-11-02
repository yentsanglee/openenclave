// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "hostfs.h"
#include <openenclave/internal/calls.h>
#include <openenclave/internal/hostsyscall.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hostbatch.h"
#include "raise.h"
#include "trace.h"

#define HOSTFS_MAGIC 0xff646572

#define BATCH_CAPACITY 4096

#define FILE_MAGIC 0xb5c3c9db

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
    fs_host_batch_t* batch = NULL;
    typedef oe_host_syscall_args_t args_t;
    args_t* args;

    if (nwritten)
        *nwritten = 0;

    /* Check parameters */
    if (!_valid_file(file) || (!data && size) || !nwritten)
        RAISE(FS_EINVAL);

    batch = file->hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->num = OE_SYSCALL_write;
        args->ret = -1;
        args->err = 0;
        args->u.close.fd = file->fd;

        if (size)
        {
            if (!(args->u.write.buf = fs_host_batch_malloc(batch, size)))
                goto done;

            memcpy(args->u.write.buf, data, size);
        }

        args->u.write.count = size;
    }

    /* Perform the OCALL. */
    {
        if (oe_ocall(OE_OCALL_HOST_SYSCALL, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if (args->ret < 0)
            RAISE(args->err);
    }

    *nwritten = args->ret;

done:

    if (batch)
        fs_host_batch_free(batch);

    return err;
}

static fs_errno_t _fs_close(fs_file_t* file)
{
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef oe_host_syscall_args_t args_t;
    args_t* args;

    if (!_valid_file(file))
        RAISE(FS_EINVAL);

    batch = file->hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->num = OE_SYSCALL_close;
        args->ret = -1;
        args->err = 0;
        args->u.close.fd = file->fd;
    }

    /* Perform the OCALL. */
    {
        if (oe_ocall(OE_OCALL_HOST_SYSCALL, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if (args->ret != 0)
            RAISE(args->err);
    }

    /* Release the file struct. */
    memset(file, 0, sizeof(fs_file_t));
    free(file);

done:

    if (batch)
        fs_host_batch_free(batch);

    return err;
}

static fs_errno_t _fs_release(fs_t* fs)
{
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;

    if (!_valid_fs(fs))
        RAISE(FS_EINVAL);

    fs_host_batch_delete(hostfs->batch);

    free(fs);

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
    hostfs_t* hostfs = (hostfs_t*)fs;
    fs_errno_t err = FS_EOK;
    fs_host_batch_t* batch = NULL;
    typedef oe_host_syscall_args_t args_t;
    args_t* args;
    fs_file_t* file = NULL;

    if (file_out)
        *file_out = NULL;

    if (!_valid_fs(fs) || !path || !file_out)
        RAISE(FS_EINVAL);

    batch = hostfs->batch;

    /* Create the arguments. */
    {
        if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->num = OE_SYSCALL_open;
        args->ret = -1;
        args->err = 0;

        if (!(args->u.open.pathname = fs_host_batch_strdup(batch, path)))
            goto done;

        args->u.open.flags = flags;
        args->u.open.mode = mode;
    }

    /* Perform the OCALL. */
    {
        if (oe_ocall(OE_OCALL_HOST_SYSCALL, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if (args->ret < 0)
            RAISE(args->err);
    }

    /* Create the file struct. */
    {
        if (!(file = calloc(1, sizeof(fs_file_t))))
            goto done;

        file->magic = FILE_MAGIC;
        file->hostfs = hostfs;
        file->fd = args->ret;
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
    fs_host_batch_t* batch = NULL;

    if (fs_out)
        *fs_out = NULL;

    if (!fs_out)
        RAISE(FS_EINVAL);

    if (!(hostfs = calloc(1, sizeof(hostfs_t))))
        RAISE(FS_ENOMEM);

    if (!(batch = fs_host_batch_new(BATCH_CAPACITY)))
        goto done;

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
