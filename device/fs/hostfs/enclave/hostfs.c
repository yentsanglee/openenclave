// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE

// clang-format off
#include <openenclave/enclave.h>
// clang-format on

#include <openenclave/internal/device.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/enclavelibc.h>
#include "../../common/hostbatch.h"
#include "../common/hostfsargs.h"

/*
**==============================================================================
**
** temporary fd table:
** ATTN: change when device interface is ready.
**
**==============================================================================
*/

#define MAX_DEVICES 4096

static oe_device_t* _devices[MAX_DEVICES];
static oe_spinlock_t _devices_lock;

static bool _valid_fd(int fd, bool obtain_lock)
{
    bool ret = false;

    if (obtain_lock)
        oe_spin_lock(&_devices_lock);

    if (fd < 0 || fd >= MAX_DEVICES || _devices[fd] == NULL)
        goto done;

    ret = true;

done:

    if (obtain_lock)
        oe_spin_unlock(&_devices_lock);

    return ret;
}

static int _assign_fd(oe_device_t* device)
{
    int ret = -1;

    oe_spin_lock(&_devices_lock);

    for (int fd = 0; fd < MAX_DEVICES; fd++)
    {
        if (_devices[fd] == NULL)
        {
            _devices[fd] = device;
            ret = fd;
            goto done;
        }
    }

done:
    oe_spin_unlock(&_devices_lock);
    return ret;
}

static int _release_fd(int fd)
{
    int ret = -1;

    oe_spin_lock(&_devices_lock);

    if (!_valid_fd(fd, false))
        goto done;

    _devices[fd] = NULL;
    ret = 0;

done:
    oe_spin_unlock(&_devices_lock);
    return ret;
}

static oe_device_t* _get_device(int fd)
{
    oe_device_t* ret = NULL;

    oe_spin_lock(&_devices_lock);

    if (fd < 0 || fd >= MAX_DEVICES)
       goto done;

    ret = _devices[fd];

done:
    oe_spin_unlock(&_devices_lock);
    return ret;
}

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

#define OE_HOSTFS_MAGIC 0x5f35f964

typedef struct _device
{
    oe_device_t base;
    uint32_t magic;
    int host_fd;
}
device_t;

typedef oe_hostfs_args_t args_t;

static device_t* _cast_device(const oe_device_t* device)
{
    device_t* dev = (device_t*)device;

    if (dev == NULL || dev->magic != OE_HOSTFS_MAGIC)
        return NULL;

    return dev;
}

static int _hostfs_close(oe_device_t* device, int fd)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    device_t* file = _cast_device(_get_device(fd));
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    oe_errno = 0;

    /* Check parameters. */
    if (!fs || !file || !batch || !_valid_fd(fd, true))
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

    if (_release_fd(fd) != 0)
    {
        oe_errno = OE_EBADF;
        goto done;
    }

    ret = 0;

done:
    return ret;
}

static int _hostfs_open(
    oe_device_t* device,
    const char *pathname,
    int flags,
    oe_mode_t mode)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    args_t* args = NULL;
    device_t* file = NULL;
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

        if (oe_strlcpy(
            args->u.open.pathname, pathname, OE_PATH_MAX) >= OE_PATH_MAX)
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
        if (!(file = oe_calloc(1, sizeof(device_t))))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        file->magic = OE_HOSTFS_MAGIC;
        file->base.ops.fs = fs->base.ops.fs;
        file->host_fd = args->u.open.ret;
    }

    /* Assign a local file descriptor for this device. */
    if ((ret = _assign_fd(&file->base)) == -1)
    {
        oe_errno = OE_ENFILE;
        goto done;
    }

    file = NULL;

done:

    if (file)
        oe_free(file);

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

oe_device_t* new_hostfs(void)
{
    device_t* ret = NULL;

    if ((ret = oe_calloc(1, sizeof(device_t))))
        goto done;

    ret->base.type = OE_DEV_HOST_FILE;
    ret->base.size = sizeof(device_t);
    ret->base.ops.fs.open = _hostfs_open;
    ret->base.ops.fs.base.close = _hostfs_close;
    ret->magic = OE_HOSTFS_MAGIC;
    ret->host_fd = -1;

    /* ATTN: finish setting operations. */
    /* ATTN: size field is not needed. */
    /* ATTN: OE_DEV_HOST_FILE not needed: should be just OE_DEV_FILE. */

done:
    return &ret->base;
}
