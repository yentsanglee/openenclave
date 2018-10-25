// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_MOUNT_H
#define _OE_MOUNT_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/result.h>

OE_EXTERNC_BEGIN

typedef enum _oe_mount_type {
    OE_MOUNT_TYPE_EXT2,
} oe_mount_type_t;

oe_result_t oe_mount(
    oe_mount_type_t type,
    const char* device_name,
    const char* path);

oe_result_t oe_unmount(const char* path);

OE_EXTERNC_END

#endif /* _OE_MOUNT_H */
