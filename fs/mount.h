// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_MOUNT_H
#define _FS_MOUNT_H

#include "common.h"

int fs_mount_hostfs(const char* target);

int fs_mount_ramfs(const char* target, uint32_t flags, size_t nblks);

int fs_mount_oefs(
    const char* source,
    const char* target,
    uint32_t flags,
    size_t nblks,
    const uint8_t key[FS_KEY_SIZE]);

#endif /* _FS_MOUNT_H */
