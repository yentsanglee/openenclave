// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE

// clang-format off
#include <openenclave/enclave.h>
// clang-format on

#include <openenclave/internal/device.h>
#include <openenclave/internal/fs_ops.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/enclavelibc.h>
#include "../../common/hostbatch.h"
#include "../common/consoleargs.h"

/*
**==============================================================================
**
** console operations:
**
**==============================================================================
*/

#define FS_MAGIC 0x5f35f964
#define FILE_MAGIC 0xfe48c6ff
#define DIR_MAGIC 0x8add1b0b

typedef oe_console_args_t args_t;

typedef struct _fs
{
    oe_device_t base;
    uint32_t magic;
} fs_t;

typedef struct _file
{
    oe_device_t base;
    uint32_t magic;
    int host_fd;
} file_t;

typedef struct _dir
{
    oe_device_t base;
    uint32_t magic;
    void* host_dir;
    struct oe_dirent entry;
} dir_t;

static fs_t* _cast_fs(const oe_device_t* device)
{
    fs_t* fs = (fs_t*)device;

    if (fs == NULL || fs->magic != FS_MAGIC)
        return NULL;

    return fs;
}

static file_t* _cast_file(const oe_device_t* device)
{
    file_t* file = (file_t*)device;

    if (file == NULL || file->magic != FILE_MAGIC)
        return NULL;

    return file;
}

static dir_t* _cast_dir(const oe_device_t* device)
{
    dir_t* dir = (dir_t*)device;

    if (dir == NULL || dir->magic != DIR_MAGIC)
        return NULL;

    return dir;
}

static ssize_t _console_read(oe_device_t*, void* buf, size_t count);

static int _console_close(oe_device_t*);

static oe_device_t* _console_open(
    oe_device_t* fs_,
    const char* pathname,
    int flags,
    oe_mode_t mode)
{
    oe_device_t* ret = NULL;
    fs_t* fs = _cast_fs(fs_);
    args_t* args = NULL;
    file_t* file = NULL;
    oe_host_batch_t* batch = _get_host_batch();

    oe_errno = 0;

    /* Check parameters */
    if (!fs || !pathname || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        args->op = OE_HOSTFS_OP_OPEN;
        args->u.open.ret = -1;

        if (oe_strlcpy(args->u.open.pathname, pathname, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }

        args->u.open.flags = flags;
        args->u.open.mode = mode;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (args->u.open.ret < 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    /* Output */
    {
        if (!(file = oe_calloc(1, sizeof(file_t))))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        file->base.type = OE_DEV_HOST_FILE;
        file->base.size = sizeof(file_t);
        file->magic = FILE_MAGIC;
        file->base.ops.fs = fs->base.ops.fs;
        file->host_fd = args->u.open.ret;
    }

    ret = &file->base;
    file = NULL;

done:

    if (file)
        oe_free(file);

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static ssize_t _console_read(oe_device_t* file_, void* buf, size_t count)
{
    int ret = -1;

    // Don't have a host read yet.

done:
    return ret;
}

static ssize_t _console_write(oe_device_t* file_, const void* buf, size_t count)
{
    int ret = -1;
    file_t* file = _cast_file(file_);
    oe_errno = 0;

    /* Check parameters. */
    if (!file || !batch || (count && !buf))
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = oe_chost_write(file->host_fd, buf, count);

done:
    return ret;
}

static oe_off_t _console_lseek(oe_device_t* file_, oe_off_t offset, int whence)
{
    oe_off_t ret = -1;

    // Not implemented

done:
    return ret;
}

static int _console_close(oe_device_t* file_)
{
    int ret = 0;
    file_t* file = _cast_file(file_);

done:
    return ret;
}

static int _console_ioctl(oe_device_t* file, unsigned long request, ...)
{
    /* Unsupported */
    oe_errno = OE_ENOTTY;
    (void)file;
    (void)request;
    return -1;
}

static oe_fs_ops_t _ops = {
    .base.ioctl = _console_ioctl,
    .open = _console_open,
    .base.read = _console_read,
    .base.write = _console_write,
    .lseek = NULL,
    .base.close = _console_close,
    .opendir = NULL,
    .readdir = NULL,
    .closedir = NULL,
    .stat = NULL,
    .link = NULL,
    .unlink = NULL,
    .rename = NULL,
    .truncate = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
};

static fs_t _console = {
    .base.type = OE_DEVICE_CONSOLE,
    .base.size = sizeof(fs_t),
    .base.ops.fs = &_ops,
    .magic = FS_MAGIC,
};

oe_device_t* oe_get_console(void)
{
    return &_console.base;
}
