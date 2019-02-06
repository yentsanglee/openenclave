// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE

// clang-format off
#include <openenclave/enclave.h>
// clang-format on

#include <openenclave/internal/device.h>
#include <openenclave/internal/epoll_ops.h>
#include <openenclave/internal/host_epoll.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/print.h>
#include "../../../fs/common/hostbatch.h"
#include "../common/epollargs.h"

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
    const size_t BATCH_SIZE = sizeof(oe_epoll_args_t) + OE_BUFSIZ;

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
** epoll operations:
**
**==============================================================================
*/

#define EPOLL_MAGIC 0x45706f6

typedef oe_epoll_args_t args_t;

typedef struct _epoll
{
    struct _oe_device base;
    uint32_t magic;
    int64_t host_fd;
    uint64_t ready_mask;
    int max_event_fds;
    int num_event_fds;
    // oe_event_device_t *event_fds;
} epoll_dev_t;

static epoll_dev_t* _cast_epoll(const oe_device_t* device)
{
    epoll_dev_t* epoll = (epoll_dev_t*)device;

    if (epoll == NULL || epoll->magic != EPOLL_MAGIC)
        return NULL;

    return epoll;
}

static epoll_dev_t _epoll;

static int _epoll_close(oe_device_t*);

static int _epoll_clone(oe_device_t* device, oe_device_t** new_device)
{
    int ret = -1;
    epoll_dev_t* epoll = _cast_epoll(device);
    epoll_dev_t* new_epoll = NULL;

    if (!epoll || !new_device)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (!(new_epoll = oe_calloc(1, sizeof(epoll_dev_t))))
    {
        oe_errno = OE_ENOMEM;
        goto done;
    }

    oe_memcpy(new_epoll, epoll, sizeof(epoll_dev_t));

    *new_device = &new_epoll->base;
    ret = 0;

done:
    return ret;
}

static int _epoll_release(oe_device_t* device)
{
    int ret = -1;
    epoll_dev_t* epoll = _cast_epoll(device);

    if (!epoll)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    oe_free(epoll);
    ret = 0;

done:
    return ret;
}

static oe_device_t* _epoll_create(oe_device_t* epoll_, int size)
{
    oe_device_t* ret = NULL;
    epoll_dev_t* epoll = NULL;

    oe_errno = 0;
    if (!batch)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    (void)_epoll_clone(epoll_, &ret);

done:
    return ret;
}

static int _epoll_ctl_add(
    oe_device_t* epoll_,
    oe_device_t* pdev,
    struct oe_epoll_event* event)
{
    int ret = -1;
    epoll_dev_t* epoll = _cast_epoll(epoll_);
    oe_device_t* pdevice = oe_get_fd_device(fd);

    oe_errno = 0;
    /* Check parameters. */
    if (!epoll_ || !pdev)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return ret;
}

static int _epoll_wait(oe_device_t* epoll_, int timeout)
{
    int ret = -1;
    epoll_dev_t* epoll = _cast_epoll(epoll_);

    oe_errno = 0;

    /* Check parameters. */
    if (!epoll_)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = 0;

done:
    return ret;
}

static int _epoll_close(oe_device_t* epoll_)
{
    int ret = -1;
    epoll_dev_t* epoll = _cast_epoll(epoll_);

    oe_errno = 0;

    /* Check parameters. */
    if (!epoll_)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Release the epoll_ object. */
    oe_free(epoll);

    ret = 0;

done:
    return ret;
}

static int _epoll_shutdown_device(oe_device_t* epoll_)
{
    int ret = -1;
    epoll_dev_t* epoll = _cast_epoll(epoll_);

    oe_errno = 0;

    /* Check parameters. */
    if (!epoll_)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Release the epoll_ object. */
    oe_free(epoll);

    ret = 0;

done:
    return ret;
}

static int _epoll_notify(oe_device_t* epoll_, uint64_t notification_mask)
{
    epoll_dev_t* epoll = _cast_epoll(epoll_);

    if (epoll->ready_mask != notification_mask)
    {
        // We notify any epolls in progress.
    }
    epoll->ready_mask = notification_mask;
    return 0;
}

static ssize_t _epoll_gethostfd(oe_device_t* epoll_)
{
    epoll_dev_t* epoll = _cast_epoll(epoll_);
    return epoll->host_fd;
}

static uint64_t _epoll_readystate(oe_device_t* epoll_)
{
    epoll_dev_t* epoll = _cast_epoll(epoll_);
    return epoll->ready_mask;
}

static oe_epoll_ops_t _ops = {
    .base.clone = _epoll_clone,
    .base.release = _epoll_release,
    .base.ioctl = NULL,
    .base.read = NULL,
    .base.write = NULL,
    .base.close = _epoll_close,
    .base.notify = _epoll_notify,
    .base.get_host_fd = _epoll_gethostfd,
    .base.ready_state = _epoll_readystate,
    .base.shutdown = _epoll_shutdown_device,
    .create = _epoll_create,
    .create1 = _epoll_create1,
    .ctl = _epoll_ctl,
    .wait = _epoll_wait,
};

static epoll_dev_t _epoll = {
    .base.type = OE_DEVICE_ID_EPOLL,
    .base.size = sizeof(epoll_dev_t),
    .base.ops.epoll = &_ops,
    .magic = EPOLL_MAGIC,
    .ready_mask = 0,
    .max_event_fds = 0,
    .num_event_fds = 0,
    // oe_event_device_t *event_fds;
};

oe_device_t* oe_epoll_get_epoll(void)
{
    return &_epoll.base;
}
