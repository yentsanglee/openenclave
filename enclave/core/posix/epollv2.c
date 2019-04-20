// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <openenclave/enclave.h>
// clang-format on

#include <openenclave/corelibc/sys/epoll.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/internal/trace.h>
#include "oe_t.h"

#define MAGIC 0x6eb39598

typedef struct _epoll
{
    struct _oe_device base;
    uint32_t magic;
    int host_fd;
} epoll_t;

static int _get_host_fd(oe_device_t* dev)
{
    int ret = -1;

    if (!dev->ops.base->get_host_fd)
    {
        oe_errno = EINVAL;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    if ((ret = (int)(*dev->ops.base->get_host_fd)(dev)) == -1)
    {
        oe_errno = EINVAL;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    oe_printf("HOSTFD=%d\n", ret);

    ret = 0;

done:
    return ret;
}

static epoll_t* _cast_epoll(const oe_device_t* device)
{
    epoll_t* epoll = (epoll_t*)device;

    if (epoll == NULL || epoll->magic != MAGIC)
        return NULL;

    return epoll;
}

static epoll_t* _fd_to_epoll(int fd)
{
    oe_device_t* dev = oe_get_fd_device(fd, OE_DEVICE_TYPE_EPOLL);

    if (!dev)
        return NULL;

    return _cast_epoll(dev);
}

static int _epoll_close(oe_device_t* dev)
{
    int ret = -1;
    epoll_t* epoll = _cast_epoll(dev);
    int err = 0;

    if (!epoll)
    {
        oe_errno = EINVAL;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    if (oe_posix_close_ocall(&ret, epoll->host_fd, &err) != OE_OK)
    {
        oe_errno = err;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    if (ret == 0)
        oe_free(epoll);

done:
    return ret;
}

static oe_device_ops_t _ops = {
    .close = _epoll_close,
};

static oe_device_t* _new_epoll_device(int host_fd)
{
    epoll_t* ret = NULL;
    epoll_t* epoll = NULL;

    if (host_fd < 0)
    {
        oe_errno = EBADF;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    if (!(epoll = oe_calloc(1, sizeof(epoll_t))))
    {
        oe_errno = ENOMEM;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    epoll->base.type = OE_DEVICE_TYPE_EPOLL;
    epoll->base.ops.base = &_ops;
    epoll->magic = MAGIC;
    epoll->host_fd = host_fd;

    ret = epoll;
    epoll = NULL;

done:

    if (epoll)
        oe_free(epoll);

    return &ret->base;
}

static int _epoll_create(
    oe_result_t (*func)(int* retval, int arg, int* err),
    int arg)
{
    int ret = -1;
    int retval = -1;
    int host_fd = -1;
    int err = 0;
    oe_device_t* epoll = NULL;
    int epfd;

    if ((*func)(&retval, arg, &err) != OE_OK)
    {
        oe_errno = EINVAL;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    if (retval == -1)
    {
        oe_errno = err;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    host_fd = retval;

    if (!(epoll = _new_epoll_device(host_fd)))
    {
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    if ((epfd = oe_assign_fd_device(epoll)) == -1)
    {
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    ret = epfd;
    epoll = NULL;
    host_fd = -1;

done:

    if (epoll)
        (*epoll->ops.base->close)(epoll);

    if (host_fd != -1)
        oe_posix_close_ocall(&retval, host_fd, &err);

    return ret;
}

int oe_epoll_create(int size)
{
    return _epoll_create(oe_posix_epollv2_create_ocall, size);
}

int oe_epoll_create1(int flags)
{
    return _epoll_create(oe_posix_epollv2_create1_ocall, flags);
}

int oe_epoll_ctl(int epfd, int op, int fd, struct oe_epoll_event* event)
{
    int ret = -1;
    epoll_t* epoll = _fd_to_epoll(epfd);
    oe_device_t* dev = oe_get_fd_device(fd, OE_DEVICE_TYPE_NONE);
    int host_fd;

    if (!epoll || !dev)
    {
        oe_errno = EINVAL;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    if ((host_fd = _get_host_fd(dev)) == -1)
    {
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    /* Call the host. */
    {
        int retval = -1;
        int err = 0;

        if (oe_posix_epollv2_ctl_ocall(
                &retval,
                epoll->host_fd,
                op,
                host_fd,
                (struct epoll_event*)event,
                &err) != OE_OK)
        {
            oe_errno = EINVAL;
            OE_TRACE_ERROR("oe_errno=%d", oe_errno);
            goto done;
        }

        if (retval == -1)
        {
            oe_errno = err;
            OE_TRACE_ERROR(
                "oe_errno=%d epoll_host_fd=%d host_fd=%d",
                oe_errno,
                epoll->host_fd,
                host_fd);
            goto done;
        }

        ret = retval;
    }

done:
    return ret;
}

int oe_epoll_wait(
    int epfd,
    struct oe_epoll_event* events,
    int maxevents,
    int timeout)
{
    int ret = -1;
    epoll_t* epoll = _fd_to_epoll(epfd);

    if (!epoll)
    {
        oe_errno = EINVAL;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    /* Call the host. */
    {
        int retval = -1;
        int err = 0;

        if (oe_posix_epollv2_wait_ocall(
                &retval,
                epoll->host_fd,
                (struct epoll_event*)events,
                (uint32_t)maxevents,
                timeout,
                &err) != OE_OK)
        {
            oe_errno = EINVAL;
            OE_TRACE_ERROR("oe_errno=%d", oe_errno);
            goto done;
        }

        if (retval == -1)
        {
            oe_errno = err;
            OE_TRACE_ERROR("oe_errno=%d", oe_errno);
            goto done;
        }

        ret = retval;
    }

done:
    return ret;
}

int oe_epoll_pwait(
    int epfd,
    struct oe_epoll_event* events,
    int maxevents,
    int timeout,
    const oe_sigset_t* sigmask)
{
    int ret = -1;

    if (sigmask)
    {
        oe_errno = ENOTSUP;
        OE_TRACE_ERROR("oe_errno=%d", oe_errno);
        goto done;
    }

    ret = oe_epoll_wait(epfd, events, maxevents, timeout);

done:
    return ret;
}
