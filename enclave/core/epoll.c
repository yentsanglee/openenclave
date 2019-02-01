// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <openenclave/enclave.h>
#include <openenclave/internal/thread.h>
// clang-format on

#include <openenclave/internal/fs.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/epoll.h>

int oe_epoll_create(int size)
{
    int ed = -1;
    oe_device_t* pepoll = NULL;
    oe_device_t* pdevice = NULL;

    pdevice = oe_get_devid_device(OE_DEVICE_ID_EPOLL);
    if ((pepoll = (*pdevice->ops.epoll->create)(size)) == NULL)
    {
        return -1;
    }
    ed = oe_allocate_fd();
    if (ed < 0)
    {
        // Log error here
        return -1; // errno is already set
    }
    pepoll = oe_set_fd_device(ed, pepoll);
    if (!pepoll)
    {
        // Log error here
        return -1; // erno is already set
    }

    return ed;
}

int oe_epoll_create1(int flags)
{
    int ed = -1;
    oe_device_t* pepoll = NULL;
    oe_device_t* pdevice = NULL;

    pdevice = oe_get_devid_device(OE_DEVICE_ID_EPOLL);
    if ((pepoll = (*pdevice->ops.epoll->create1)(flags)) == NULL)
    {
        return -1;
    }
    ed = oe_allocate_fd();
    if (ed < 0)
    {
        // Log error here
        return -1; // errno is already set
    }
    pepoll = oe_set_fd_device(ed, pepoll);
    if (!pepoll)
    {
        // Log error here
        return -1; // erno is already set
    }

    return ed;
}

int oe_epoll_ctl(int epfd, int op, int fd, struct oe_epoll_event* event)
{
    oe_device_t* pepoll = oe_get_fd_device(epfd);

    if (!pepoll)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (pepoll->ops.epoll->ctl == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*pepoll->ops.epoll->ctl)(pepoll, op, fd, event);
}

int oe_epoll_wait(
    int epfd,
    struct oe_epoll_event* events,
    int maxevents,
    int timeout)
{
    oe_device_t* pepoll = oe_get_fd_device(epfd);

    if (!pepoll)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (pepoll->ops.epoll->wait == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*pepoll->ops.epoll->wait)(pepoll, events, maxevents, timeout);
}

#if MAYBE
int oe_epoll_pwait(
    int epfd,
    struct epoll_event* events,
    int maxevents,
    int timeout,
    const sigset_t* ss)
{
    return -1;
}

#endif
