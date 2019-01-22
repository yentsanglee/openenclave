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
    /* What calls this? */
    int (*init)();

    /* What calls this? */
    int (*create)();

    /* What calls this? */
    int (*remove)();

    ssize_t (*read)(oe_device_t* dev, void* buf, size_t count);

    ssize_t (*write)(oe_device_t* dev, const void* buf, size_t count);

    int (*close)(oe_device_t* dev);

    int (*ioctl)(oe_device_t* dev, unsigned long request, ...);

} oe_device_ops_t;

OE_EXTERNC_END

#endif // _OE_DEVICE_OPS_H
