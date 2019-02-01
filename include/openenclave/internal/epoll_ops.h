
/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef __OE_EPOLL_OPS_H__
#define __OE_EPOLL_OPS_H__

#include <openenclave/internal/device_ops.h>

#ifdef cplusplus
extern "C"
{
#endif

    struct oe_epoll_event;

    typedef struct _oe_device oe_device_t;

    typedef struct _oe_epoll_ops
    {
        oe_device_ops_t base;
        oe_device_t* (*create)(int size);
        oe_device_t* (*create1)(int flags);
        int (*ctl_add)( oe_device_t* pepoll, oe_device_t* pdev, struct oe_epoll_event* event);
        int (*ctl_del)( oe_device_t* pepoll, oe_device_t* pdev, struct oe_epoll_event* event);
        int (*ctl_mod)( oe_device_t* pepoll, oe_device_t* pdev, struct oe_epoll_event* event);
        int (*wait)( oe_device_t* pepoll, int timeout);
    } oe_epoll_ops_t;

#ifdef cplusplus
}
#endif

#endif
