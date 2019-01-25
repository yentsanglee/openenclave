// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE

// clang-format off
#include <openenclave/enclave.h>
// clang-format on

#include <openenclave/internal/device.h>
#include <openenclave/internal/sock_ops.h>
#include <openenclave/internal/host_socket.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/print.h>
#include "../../../fs/common/hostbatch.h"
#include "../common/hostsockargs.h"

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
** hostsock operations:
**
**==============================================================================
*/

#define SOCKET_MAGIC 0x536f636b

typedef oe_hostfs_args_t args_t;

typedef struct _sock
{
    struct _oe_device base;
    uint32_t magic;
    int64_t host_fd;
    int ready;
} sock_t;

static sock_t* _cast_sock(const oe_device_t* device)
{
    sock_t* sock = (sock_t*)device;

    if (sock == NULL || sock->magic != SOCKET_MAGIC)
        return NULL;

    return sock;
}

static sock_t _hostsock;
static ssize_t _hostsock_read(oe_device_t*, void* buf, size_t count);

// static int _hostsock_close(oe_device_t*);

static int _hostsock_clone(oe_device_t* device, oe_device_t** new_device)
{
    int ret = -1;
    sock_t* sock = _cast_sock(device);
    sock_t* new_sock = NULL;

    if (!sock || !new_device)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (!(new_sock = oe_calloc(1, sizeof(sock_t))))
    {
        oe_errno = OE_ENOMEM;
        goto done;
    }

    oe_memcpy(new_sock, sock, sizeof(sock_t));

    *new_device = &new_sock->base;
    ret = 0;

done:
    return ret;
}

static int _hostsock_release(oe_device_t* device)
{
    int ret = -1;
    sock_t* sock = _cast_sock(device);

    if (!sock)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    oe_free(sock);
    ret = 0;

done:
    return ret;
}

static int _hostsock_shutdown(oe_device_t* device)
{
    int ret = -1;
    sock_t* sock = _cast_sock(device);

    if (!sock)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = 0;

done:
    return ret;
}

static oe_device_t* _hostsock_socket(
    oe_device_t* sock_,
    int domain,
    int type,
    int protocol)
{
    oe_device_t* ret = NULL;
    sock_t* sock = NULL;
    args_t* args = NULL;
    oe_host_batch_t* batch = _get_host_batch();

    oe_errno = 0;

    if (!batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    (void)_hostsock_clone(sock_, &ret);
    sock = _cast_sock(ret);
    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        args->op = OE_HOSTSOCK_OP_SOCKET;
        args->u.socket.ret = -1;

        args->u.socket.domain = domain;
        args->u.socket.type = type;
        args->u.socket.protocol = protocol;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTSOCK, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (args->u.socket.ret < 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    /* Output */
    {
        sock->base.type = OE_DEVICE_HOST_SOCKET;
        sock->base.size = sizeof(sock_t);
        sock->magic = SOCKET_MAGIC;
        sock->base.ops.socket = _hostsock.base.ops.socket;
        sock->host_fd = args->u.socket.ret;
    }

    sock = NULL;

done:

    if (sock)
        oe_free(sock);

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static ssize_t _hostsock_read(oe_device_t* sock_, void* buf, size_t count)
{
    ssize_t ret = -1;
    sock_t* sock = _cast_sock(sock_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    oe_errno = 0;

    /* Check parameters. */
    if (!sock || !batch || (count && !buf))
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

        args->op = OE_HOSTSOCK_OP_RECV;
        args->u.recv.ret = -1;
        args->u.recv.host_fd = sock->host_fd;
        args->u.recv.count = count;
        args->u.recv.flags = 0;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTSOCK, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if ((ret = args->u.recv.ret) == -1)
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

static ssize_t _hostsock_write(
    oe_device_t* sock_,
    const void* buf,
    size_t count)
{
    ssize_t ret = -1;
    sock_t* sock = _cast_sock(sock_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    oe_errno = 0;

    /* Check parameters. */
    if (!sock || !batch || (count && !buf))
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

        args->op = OE_HOSTSOCK_OP_SEND;
        args->u.send.ret = -1;
        args->u.send.host_fd = sock->host_fd;
        args->u.send.count = count;
        args->u.send.flags = 0;
        oe_memcpy(args->buf, buf, count);
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTSOCK, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if ((ret = args->u.send.ret) == -1)
        {
            oe_errno = args->err;
            goto done;
        }
    }

done:
    return ret;
}

#if 0
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

        dir->base.type = OE_DEVICE_HOST_FILESYSTEM;
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
#endif

static oe_sock_ops_t _ops = {
    .base.clone = _hostsock_clone,
    .base.release = _hostsock_release,
    .base.shutdown = _hostsock_shutdown,
    //    .base.ioctl  = _hostsock_ioctl,
    .base.read = _hostsock_read,
    .base.write = _hostsock_write,
    //    .base.close  = _hostsock_close,
    .socket = _hostsock_socket,
    //    .connect     = _hostsock_connect,
    //    .accept      = _hostsock_accept,
    //    .bind        = _hostsock_bind,
    //    .listen      = _hostsock_listen,
    //    .recvmsg     = _hostsock_recvmsg,
    //    .sendmsg     = _hostsock_sendmsg,
    //    .shutdown    = _hostsock_shutdown,
    //    .getsockopt  = _hostsock_getsockopt,
    //    .setsockopt  = _hostsock_setsockopt,
    //    .getpeername = _hostsock_getpeername,
    //    .getsockname = _hostsock_getsockname
};

static sock_t _hostsock = {
    .base.type = OE_DEVICE_HOST_SOCKET,
    .base.size = sizeof(sock_t),
    .base.ops.socket = &_ops,
    .magic = SOCKET_MAGIC,
};

oe_device_t* oe_socket_get_hostsock(void)
{
    return &_hostsock.base;
}
