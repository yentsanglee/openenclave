// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <openenclave/enclave.h>
#include <openenclave/internal/thread.h>
// clang-format on

#include <openenclave/internal/utils.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/epoll.h>

int oe_epoll_create(int size)
{
    int ed = -1;
    oe_device_t* pepoll = NULL;
    oe_device_t* pdevice = NULL;

    pdevice = oe_get_devid_device(OE_DEVID_EPOLL);
    if ((pepoll = (*pdevice->ops.epoll->create)(pdevice, size)) == NULL)
    {
        return -1;
    }
    ed = oe_assign_fd_device(pepoll);
    if (ed == -1)
    {
        // ATTN: release pepoll here.
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

    pdevice = oe_get_devid_device(OE_DEVID_EPOLL);
    if ((pepoll = (*pdevice->ops.epoll->create1)(pdevice, flags)) == NULL)
    {
        return -1;
    }
    ed = oe_assign_fd_device(pepoll);
    if (ed == -1)
    {
        // ATTN: release pepoll here.
        // Log error here
        return -1; // erno is already set
    }

    return ed;
}

int oe_epoll_ctl(int epfd, int op, int fd, struct oe_epoll_event* event)
{
    int ret = -1;
    oe_device_t* pepoll = oe_get_fd_device(epfd);
    oe_device_t* pdevice = oe_get_fd_device(fd);

    oe_errno = 0;
    /* Check parameters. */
    if (!pepoll || !pdevice)
    {
        oe_errno = EBADF;
        return -1;
    }

    switch (op)
    {
        case OE_EPOLL_CTL_ADD:
        {
            if (pepoll->ops.epoll->ctl_add == NULL)
            {
                oe_errno = EINVAL;
                return -1;
            }
            ret = (*pepoll->ops.epoll->ctl_add)(pepoll, fd, event);
            break;
        }
        case OE_EPOLL_CTL_DEL:
        {
            ret = (*pepoll->ops.epoll->ctl_del)(pepoll, fd);
            break;
        }
        case OE_EPOLL_CTL_MOD:
        {
            ret = (*pepoll->ops.epoll->ctl_del)(pepoll, fd);
            break;
        }
        default:
        {
            oe_errno = EINVAL;
            return -1;
        }
    }

    return ret;
}

int oe_epoll_wait(
    int epfd,
    struct oe_epoll_event* events,
    int maxevents,
    int timeout)
{
    oe_device_t* pepoll = oe_get_fd_device(epfd);
    int ret = -1;
    bool host_wait = true; // false; // 2do. We need to figure out how to wait
                           // on a host or enclave file/socket.
    (void)events;
    (void)maxevents;

    if (!pepoll)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (pepoll->ops.epoll->wait == NULL)
    {
        oe_errno = EINVAL;
        return -1;
    }

    // Start an outboard waiter if host involved
    // search polled device list for host involved  2Do
    if (host_wait)
    {
        if ((*pepoll->ops.epoll->wait)(
                pepoll, events, (size_t)maxevents, timeout) < 0)
        {
            oe_errno = EINVAL;
            return -1;
        }
    }
    ret = oe_wait_device_notification(timeout);

    // Search the list for events changed . If OE_EPOLLET is set check against
    // previous. Copy the active

    return 0; // return the number of descriptors that have signalled
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

static oe_cond_t poll_notification = OE_COND_INITIALIZER;
static oe_mutex_t poll_lock = OE_MUTEX_INITIALIZER;

//
// We accept a list of notifications so we don't get large number
// of handle notification calls in rapid succesion. This could raise needless
// synchronisaion issues. Instead, we send the list and notify the list, the
// push the doorbell
oe_result_t _handle_oe_device_notification(uint64_t arg)
{
    oe_result_t result = OE_FAILURE;
    struct oe_device_notification_args* pargs =
        (struct oe_device_notification_args*)arg;
    struct oe_device_notification_args local;
    oe_device_t* pdevice = NULL;
    uint64_t num_notifications;
    uint64_t i = 0;
    struct oe_device_notifications* pnotifications = NULL;

    if (pargs == NULL)
    {
        result = OE_INVALID_PARAMETER;
        goto done;
    }
    num_notifications = pargs->num_notifications;
    if (!oe_is_outside_enclave(
            (void*)pargs,
            sizeof(struct oe_device_notification_args) +
                (sizeof(struct oe_device_notifications) * num_notifications)))
    {
        result = OE_INVALID_PARAMETER;
        goto done;
    }

    /* Copy structure into enclave memory */
    oe_secure_memcpy(
        &local,
        pargs,
        sizeof(struct oe_device_notification_args) +
            (sizeof(struct oe_device_notifications) * num_notifications));
    pnotifications = (struct oe_device_notifications*)((&local) + 1);

    for (i = 0; i < num_notifications; i++)
    {
        /* the enclave fd was sent down to the host when we did the ctl_add. The
           epoll event structure allows tagging the
           wait event with an arbitrary int64 */
        pdevice = oe_get_fd_device((int)pnotifications[1].enclave_fd);
        if (pdevice != NULL)
        {
            /* set the event mask in the pdevice */
            (*pdevice->ops.base->notify)(
                pdevice, (uint64_t)pnotifications[i].event_mask);
        }
        else
        {
            // Log the issue
        }
    }

    /* push the doorbell */
    oe_broadcast_device_notification();

    result = OE_OK;
done:
    return result;
}

void oe_signal_device_notification(oe_device_t* pdevice, uint32_t event_mask)
{
    (void)pdevice;
    (void)event_mask;
}

void oe_broadcast_device_notification()
{
}

int oe_wait_device_notification(int timeout)
{
    (void)timeout;

    oe_mutex_lock(&poll_lock);
    oe_cond_wait(&poll_notification, &poll_lock);
    oe_mutex_unlock(&poll_lock);

    return -1;
}

void oe_clear_device_notification()
{
}
