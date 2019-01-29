// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE

// clang-format off
#include <openenclave/enclave.h>
// clang-format on

#include <openenclave/internal/device.h>
#include <openenclave/internal/fs_ops.h>
#include <openenclave/internal/fs.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/fs.h>
#include "../../common/hostbatch.h"
#include "../common/hostfsargs.h"

/*
**==============================================================================
**
** host batch:
**
**==============================================================================
*/

static oe_host_batch_t* _host_batch;
static oe_spinlock_t _lock;

static void _atexit_handler()
{
    oe_spin_lock(&_lock);
    oe_host_batch_delete(_host_batch);
    _host_batch = NULL;
    oe_spin_unlock(&_lock);
}

static oe_host_batch_t* _get_host_batch(void)
{
    const size_t BATCH_SIZE = sizeof(oe_hostfs_args_t) + OE_BUFSIZ;

    if (_host_batch == NULL)
    {
        oe_spin_lock(&_lock);

        if (_host_batch == NULL)
        {
            _host_batch = oe_host_batch_new(BATCH_SIZE);
            oe_atexit(_atexit_handler);
        }

        oe_spin_unlock(&_lock);
    }

    return _host_batch;
}

/*
**==============================================================================
**
** hostfs operations:
**
**==============================================================================
*/

#define FS_MAGIC 0x5f35f964
#define FILE_MAGIC 0xfe48c6ff
#define DIR_MAGIC 0x8add1b0b

typedef oe_hostfs_args_t args_t;

typedef struct _fs
{
    struct _oe_device base;
    uint32_t magic;
} fs_t;

typedef struct _file
{
    struct _oe_device base;
    uint32_t magic;
    int host_fd;
} file_t;

typedef struct _dir
{
    struct _oe_device base;
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

static ssize_t _hostfs_read(oe_device_t*, void* buf, size_t count);

static int _hostfs_close(oe_device_t*);

static int _hostfs_clone(oe_device_t* device, oe_device_t** new_device)
{
    int ret = -1;
    fs_t* fs = _cast_fs(device);
    fs_t* new_fs = NULL;

    if (!fs || !new_device)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (!(new_fs = oe_calloc(1, sizeof(fs_t))))
    {
        oe_errno = OE_ENOMEM;
        goto done;
    }

    oe_memcpy(new_fs, fs, sizeof(fs_t));

    *new_device = &new_fs->base;
    ret = 0;

done:
    return ret;
}

static int _hostfs_release(oe_device_t* device)
{
    int ret = -1;
    fs_t* fs = _cast_fs(device);

    if (!fs)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    oe_free(fs);
    ret = 0;

done:
    return ret;
}

static int _hostfs_shutdown(oe_device_t* device)
{
    int ret = -1;
    fs_t* fs = _cast_fs(device);

    if (!fs)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = 0;

done:
    return ret;
}

static int _hostfs_mount(
    oe_device_t* device,
    const char* target,
    uint32_t flags)
{
    int ret = -1;
    fs_t* fs = _cast_fs(device);

    if (!fs || !target)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    (void)flags;

    ret = 0;

done:
    return ret;
}

static int _hostfs_unmount(oe_device_t* device)
{
    int ret = -1;
    fs_t* fs = _cast_fs(device);

    if (!fs)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = 0;

done:
    return ret;
}

static oe_device_t* _hostfs_open(
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

        file->base.type = OE_DEVICETYPE_FILE,
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

static ssize_t _hostfs_read(oe_device_t* file_, void* buf, size_t count)
{
    int ret = -1;
    file_t* file = _cast_file(file_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    oe_errno = 0;

    /* Check parameters. */
    if (!file || !batch || (count && !buf))
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t) + count)))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        args->op = OE_HOSTFS_OP_READ;
        args->u.read.ret = -1;
        args->u.read.fd = file->host_fd;
        args->u.read.count = count;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if ((ret = args->u.open.ret) == -1)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    /* Output */
    {
        oe_memcpy(buf, args->buf, count);
    }

done:
    return ret;
}

static ssize_t _hostfs_write(oe_device_t* file_, const void* buf, size_t count)
{
    int ret = -1;
    file_t* file = _cast_file(file_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    oe_errno = 0;

    /* Check parameters. */
    if (!file || !batch || (count && !buf))
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t) + count)))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        args->op = OE_HOSTFS_OP_WRITE;
        args->u.write.ret = -1;
        args->u.write.fd = file->host_fd;
        args->u.write.count = count;
        oe_memcpy(args->buf, buf, count);
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if ((ret = args->u.open.ret) == -1)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:
    return ret;
}

static oe_off_t _hostfs_lseek(oe_device_t* file_, oe_off_t offset, int whence)
{
    oe_off_t ret = -1;
    file_t* file = _cast_file(file_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    oe_errno = 0;

    /* Check parameters. */
    if (!file || !batch)
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

        args->op = OE_HOSTFS_OP_LSEEK;
        args->u.lseek.ret = -1;
        args->u.lseek.fd = file->host_fd;
        args->u.lseek.offset = offset;
        args->u.lseek.whence = whence;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if ((ret = args->u.lseek.ret) == -1)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:
    return ret;
}

static int _hostfs_close(oe_device_t* file_)
{
    int ret = -1;
    file_t* file = _cast_file(file_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    oe_errno = 0;

    /* Check parameters. */
    if (!file || !batch)
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

        args->op = OE_HOSTFS_OP_CLOSE;
        args->u.close.ret = -1;
        args->u.close.fd = file->host_fd;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (args->u.close.ret != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    /* Release the file object. */
    oe_free(file);

    ret = 0;

done:
    return ret;
}

static int _hostfs_ioctl(
    oe_device_t* file,
    unsigned long request,
    oe_va_list ap)
{
    /* Unsupported */
    oe_errno = OE_ENOTTY;
    (void)file;
    (void)request;
    (void)ap;
    return -1;
}

static oe_device_t* _hostfs_opendir(oe_device_t* fs_, const char* name)
{
    oe_device_t* ret = NULL;
    fs_t* fs = _cast_fs(fs_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;
    dir_t* dir = NULL;

    /* Check parameters */
    if (!fs || !name || !batch)
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

        args->op = OE_HOSTFS_OP_OPENDIR;
        args->u.opendir.ret = NULL;
        oe_strlcpy(args->u.opendir.name, name, OE_PATH_MAX);
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (args->u.opendir.ret == NULL)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    /* Output */
    {
        if (!(dir = oe_calloc(1, sizeof(dir_t))))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        dir->base.type = OE_DEVICETYPE_DIRECTORY;
        dir->base.size = sizeof(dir_t);
        dir->magic = DIR_MAGIC;
        dir->base.ops.fs = fs->base.ops.fs;
        dir->host_dir = args->u.opendir.ret;
    }

    ret = &dir->base;
    dir = NULL;

done:

    if (args)
        oe_host_batch_free(batch);

    if (dir)
        oe_free(dir);

    return ret;
}

static struct oe_dirent* _hostfs_readdir(oe_device_t* dir_)
{
    struct oe_dirent* ret = NULL;
    dir_t* dir = _cast_dir(dir_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!dir || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->u.readdir.ret = NULL;
        args->op = OE_HOSTFS_OP_READDIR;
        args->u.readdir.dirp = dir->host_dir;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if (!args->u.readdir.ret)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    /* Output */
    if (args->u.readdir.ret)
    {
        dir->entry = args->u.readdir.entry;
        ret = &dir->entry;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostfs_closedir(oe_device_t* dir_)
{
    int ret = -1;
    dir_t* dir = _cast_dir(dir_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!dir || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_CLOSEDIR;
        args->u.closedir.ret = -1;
        args->u.closedir.dirp = dir->host_dir;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if ((ret = args->u.closedir.ret) != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    oe_free(dir);

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostfs_stat(
    oe_device_t* fs_,
    const char* pathname,
    struct oe_stat* buf)
{
    int ret = -1;
    fs_t* fs = _cast_fs(fs_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!fs || !pathname || !buf || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_STAT;
        args->u.stat.ret = -1;

        if (oe_strlcpy(args->u.stat.pathname, pathname, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if ((ret = args->u.stat.ret) != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    /* Output */
    {
        *buf = args->u.stat.buf;
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostfs_link(
    oe_device_t* fs_,
    const char* oldpath,
    const char* newpath)
{
    int ret = -1;
    fs_t* fs = _cast_fs(fs_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!fs || !oldpath || !newpath || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_LINK;
        args->u.link.ret = -1;

        if (oe_strlcpy(args->u.link.oldpath, oldpath, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }

        if (oe_strlcpy(args->u.link.newpath, newpath, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if ((ret = args->u.link.ret) != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostfs_unlink(oe_device_t* fs_, const char* pathname)
{
    int ret = -1;
    fs_t* fs = _cast_fs(fs_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!fs || !pathname || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_UNLINK;
        args->u.unlink.ret = -1;

        if (oe_strlcpy(args->u.unlink.pathname, pathname, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if ((ret = args->u.unlink.ret) != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostfs_rename(
    oe_device_t* fs_,
    const char* oldpath,
    const char* newpath)
{
    int ret = -1;
    fs_t* fs = _cast_fs(fs_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!fs || !oldpath || !newpath || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_RENAME;
        args->u.rename.ret = -1;

        if (oe_strlcpy(args->u.rename.oldpath, oldpath, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }

        if (oe_strlcpy(args->u.rename.newpath, newpath, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if ((ret = args->u.rename.ret) != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostfs_truncate(oe_device_t* fs_, const char* path, oe_off_t length)
{
    int ret = -1;
    fs_t* fs = _cast_fs(fs_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!fs || !path || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_TRUNCATE;
        args->u.truncate.ret = -1;
        args->u.truncate.length = length;

        if (oe_strlcpy(args->u.truncate.path, path, OE_PATH_MAX) >= OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if ((ret = args->u.truncate.ret) != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostfs_mkdir(oe_device_t* fs_, const char* pathname, oe_mode_t mode)
{
    int ret = -1;
    fs_t* fs = _cast_fs(fs_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!fs || !pathname || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_MKDIR;
        args->u.mkdir.ret = -1;
        args->u.mkdir.mode = mode;

        if (oe_strlcpy(args->u.mkdir.pathname, pathname, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if ((ret = args->u.mkdir.ret) != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostfs_rmdir(oe_device_t* fs_, const char* pathname)
{
    int ret = -1;
    fs_t* fs = _cast_fs(fs_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    /* Check parameters */
    if (!fs || !pathname || !batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
            goto done;

        args->op = OE_HOSTFS_OP_RMDIR;
        args->u.rmdir.ret = -1;

        if (oe_strlcpy(args->u.rmdir.pathname, pathname, OE_PATH_MAX) >=
            OE_PATH_MAX)
        {
            oe_errno = OE_ENAMETOOLONG;
            goto done;
        }
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTFS, (uint64_t)args, NULL) != OE_OK)
            goto done;

        if ((ret = args->u.rmdir.ret) != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static oe_fs_ops_t _ops = {
    .base.clone = _hostfs_clone,
    .base.release = _hostfs_release,
    .base.shutdown = _hostfs_shutdown,
    .base.ioctl = _hostfs_ioctl,
    .mount = _hostfs_mount,
    .unmount = _hostfs_unmount,
    .open = _hostfs_open,
    .base.read = _hostfs_read,
    .base.write = _hostfs_write,
    .lseek = _hostfs_lseek,
    .base.close = _hostfs_close,
    .opendir = _hostfs_opendir,
    .readdir = _hostfs_readdir,
    .closedir = _hostfs_closedir,
    .stat = _hostfs_stat,
    .link = _hostfs_link,
    .unlink = _hostfs_unlink,
    .rename = _hostfs_rename,
    .truncate = _hostfs_truncate,
    .mkdir = _hostfs_mkdir,
    .rmdir = _hostfs_rmdir,
};

static fs_t _hostfs = {
    .base.type = OE_DEVICETYPE_FILESYSTEM,
    .base.size = sizeof(fs_t),
    .base.ops.fs = &_ops,
    .magic = FS_MAGIC,
};

oe_device_t* oe_fs_get_hostfs(void)
{
    return &_hostfs.base;
}

int oe_fs_init_hostfs_device(void)
{
    int ret = -1;

    /* Allocate the device id. */
    if (oe_allocate_devid(OE_DEVICE_ID_HOSTFS) != OE_DEVICE_ID_HOSTFS)
        goto done;

    /* Add the hostfs device to the device table. */
    if (oe_set_devid_device(OE_DEVICE_ID_HOSTFS, oe_fs_get_hostfs()) != 0)
        goto done;

    /* Check that the above operation was successful. */
    if (oe_get_devid_device(OE_DEVICE_ID_HOSTFS) != oe_fs_get_hostfs())
        goto done;

    ret = 0;

done:
    return ret;
}
