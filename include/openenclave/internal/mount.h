// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_MOUNT_H
#define _OE_MOUNT_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/result.h>

OE_EXTERNC_BEGIN

#define OE_MOUNT_FLAG_NONE 0
#define OE_MOUNT_FLAG_MKFS 1
#define OE_MOUNT_FLAG_CRYPTO 2
#define OE_MOUNT_KEY_SIZE 32

int oe_mount_oefs(
    const char* source,
    const char* target,
    uint32_t flags,
    size_t num_blocks,
    const uint8_t key[OE_MOUNT_KEY_SIZE]);

int oe_mount_hostfs(const char* target);

int oe_unmount(const char* target);

OE_EXTERNC_END

#endif /* _OE_MOUNT_H */
