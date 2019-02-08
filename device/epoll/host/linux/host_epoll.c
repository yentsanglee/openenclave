// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <openenclave/host.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../common/epollargs.h"

void (*oe_handle_hostepoll_ocall_callback)(void*);

static void _handle_hostepoll_ocall(void* args_)
{
    oe_epoll_args_t* args = (oe_epoll_args_t*)args_;

    /* ATTN: handle errno propagation. */

    if (!args)
        return;

    args->err = 0;
    switch (args->op)
    {
        case OE_EPOLL_OP_NONE:
        {
            break;
        }
        case OE_EPOLL_OP_CREATE:
        {
            printf("host epoll create\n");
            args->u.create.ret = epoll_create1(args->u.create.flags);
            break;
        }
        case OE_EPOLL_OP_CLOSE:
        {
            args->u.close.ret = close((int)args->u.close.host_fd);
            break;
        }
        case OE_EPOLL_OP_ADD:
        {
            union _oe_ev_data ev_data = {
                .event_list_idx = (uint32_t)args->u.ctl_add.list_idx,
                .handle_type = (uint32_t)args->u.ctl_add.handle_type,
            };

            struct epoll_event ev = {
                .events = args->u.ctl_add.event_mask,
                .data.u64 = ev_data.data,
            };

            args->u.ctl_add.ret = epoll_ctl(
                (int)args->u.ctl_add.epoll_fd,
                EPOLL_CTL_ADD,
                (int)args->u.ctl_add.host_fd,
                &ev);
            break;
        }
        case OE_EPOLL_OP_MOD:
        {
            union _oe_ev_data ev_data = {
                .event_list_idx = (uint32_t)args->u.ctl_mod.list_idx,
                .handle_type = (uint32_t)args->u.ctl_mod.handle_type,
            };

            struct epoll_event ev = {
                .events = args->u.ctl_mod.event_mask,
                .data.u64 = ev_data.data,
            };

            args->u.ctl_mod.ret = epoll_ctl(
                (int)args->u.ctl_mod.epoll_fd,
                EPOLL_CTL_MOD,
                (int)args->u.ctl_mod.host_fd,
                &ev);
            break;
        }
        case OE_EPOLL_OP_DEL:
        {
            args->u.ctl_del.ret = epoll_ctl(
                (int)args->u.ctl_del.epoll_fd,
                EPOLL_CTL_DEL,
                (int)args->u.ctl_del.host_fd,
                NULL);

            // If in windows, delete auxiliary data, such as WSASocketEvents so
            // as not to leak handles.
            break;
        }
        case OE_EPOLL_OP_WAIT:
        {
            args->u.wait.ret = epoll_wait(
                (int)args->u.wait.epoll_fd,
                (struct epoll_event*)args->buf,
                (int)args->u.wait.maxevents,
                (int)args->u.wait.timeout);
            break;
        }
        case OE_EPOLL_OP_SHUTDOWN_DEVICE:
        {
            // 2do
            break;
        }
        default:
        {
            // Invalid
            break;
        }
    }
    args->err = errno;
}

void oe_epoll_install_hostepoll(void)
{
    oe_handle_hostepoll_ocall_callback = _handle_hostepoll_ocall;
}
