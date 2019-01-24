// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_DEVICE_OPS_H
#define _OE_DEVICE_OPS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/types.h>

OE_EXTERNC_BEGIN

typedef struct _oe_device oe_device_t;

typedef struct _oe_device_ops
{
    /* What calls this? */
    int (*init)(oe_device_t* pthis);

    int (*clone)(oe_device_t* device, oe_device_t** new_device);

    int (*free)(oe_device_t* device);

    /* What calls this? */
    int (*remove)(oe_device_t* pthis);

    ssize_t (*read)(oe_device_t* file, void* buf, size_t count);

    ssize_t (*write)(oe_device_t* file, const void* buf, size_t count);

    int (*close)(oe_device_t* file);

    int (*ioctl)(oe_device_t* file, unsigned long request, oe_va_list ap);

} oe_device_ops_t;

OE_EXTERNC_END

#endif // _OE_DEVICE_OPS_H
