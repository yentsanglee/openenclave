// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_MOUNT_H
#define _FS_MOUNT_H

#include "common.h"

FS_EXTERN_C_BEGIN

#define FS_MOUNT_FLAG_NONE 0
#define FS_MOUNT_FLAG_MKFS 1
#define FS_MOUNT_FLAG_CRYPTO 2
#define FS_MOUNT_KEY_SIZE 32

int fs_mount_oefs(
    const char* source,
    const char* target,
    uint32_t flags,
    size_t nblks,
    const uint8_t key[FS_MOUNT_KEY_SIZE]);

int fs_mount_hostfs(const char* target);

int fs_unmount(const char* target);

FS_EXTERN_C_END

#endif /* _FS_MOUNT_H */
