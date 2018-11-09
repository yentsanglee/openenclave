// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_FSFS_H
#define _FS_FSFS_H

#include "blkdev.h"
#include "common.h"
#include "fs.h"
#include "mount.h"

/* Compute required size of a file system with the given block count. */
fs_errno_t oefs_size(size_t nblks, size_t* size);

/* Build an OE file system on the given device. */
fs_errno_t oefs_mkfs(fs_blkdev_t* dev, size_t nblks);

/* Initialize the oefs instance from the given device. */
fs_errno_t oefs_initialize(fs_t** fs, fs_blkdev_t* dev);

int fs_mount_oefs(
    const char* source,
    const char* target,
    uint32_t flags,
    size_t nblks,
    const uint8_t key[FS_MOUNT_KEY_SIZE]);

#endif /* _FS_FSFS_H */
