// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_EPOLLARGS_H
#define _OE_EPOLLARGS_H

OE_EXTERNC_BEGIN

typedef enum _oe_epoll_op
{
    OE_EPOLL_OP_NONE,
    OE_EPOLL_OP_CREATE,
    OE_EPOLL_OP_ADD,
    OE_EPOLL_OP_DEL,
    OE_EPOLL_OP_MOD,
    OE_EPOLL_OP_WAIT,
    OE_EPOLL_OP_CLOSE,
    OE_EPOLL_OP_SHUTDOWN_DEVICE
} oe_epoll_op_t;

union _oe_ev_data {
    struct
    {
        uint32_t event_list_idx;
        uint32_t handle_type;
    };
    uint64_t data;
};

typedef struct _oe_epoll_args
{
    oe_epoll_op_t op;
    int err;
    union {
        struct
        {
            int64_t ret; // returns host_fd for the epoll
            int32_t flags;
            // the size argument is obsolete and not used
        } create;
        struct
        {
            int64_t ret; // returns numfds returned or -1
            int64_t epoll_fd;
            int64_t maxevents;
            int64_t timeout;
            // event array  returned in buf
        } wait;
        struct
        {
            int64_t ret;
            int64_t epoll_fd;
            int64_t host_fd;
            uint32_t event_mask;
            int32_t list_idx;
            int32_t handle_type;  // We need this for windows. A file handle != socket handle in win32 and its awkward to figure it out.
        } ctl_add;
        struct
        {
            int64_t ret;
            int64_t epoll_fd;  // host_fd of the epoll
            int64_t host_fd;  // host fd for the waitable
        } ctl_del;
        struct
        {
            int64_t ret;
            int64_t epoll_fd;
            int64_t host_fd;
            uint32_t event_mask;
            int32_t list_idx;
            int32_t handle_type;  // We need this for windows. A file handle != socket handle in win32 and its awkward to figure it out.
        } ctl_mod;
        struct
        {
            int64_t ret;
            int64_t host_fd;
        } close;
        struct
        {
            int64_t ret;
            int64_t host_fd;
        } shutdown_device;
    } u;
    uint8_t buf[];
} oe_epoll_args_t;

OE_EXTERNC_END

#endif /* _OE_HOSTFSARGS_H */
