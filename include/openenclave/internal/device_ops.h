// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_DEVICE_OPS_H
#define _OE_DEVICE_OPS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

typedef struct _oe_device oe_device_t;

typedef struct _oe_device_ops
{
    int (*init)();

    int (*create)();

    int (*remove)();

    /* Decrement the internal reference count and release if zero. */
    void (*release)(oe_device_t* dev);

    /* Increment the internal reference count. */
    void (*add_ref)(oe_device_t* dev);

    ssize_t (*read)(oe_device_t* dev, int fd, void* buf, size_t count);

    ssize_t (*write)(oe_device_t* dev, int fd, const void* buf, size_t count);

    int (*ioctl)(oe_device_t* dev, int fd, unsigned long request, ...);

    int (*close)(oe_device_t* dev, int fd);

} oe_device_ops_t;

OE_EXTERNC_END

#endif // _OE_DEVICE_OPS_H
