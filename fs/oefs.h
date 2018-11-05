// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_OEFS_H
#define _OE_OEFS_H

#include "blkdev.h"
#include "fs.h"

/* Compute required size of a file system with the given block count. */
fs_errno_t oefs_size(size_t nblocks, size_t* size);

/* Build an OE file system on the given device. */
fs_errno_t oefs_mkfs(fs_blkdev_t* dev, size_t nblocks);

/* Initialize the oefs instance from the given device. */
fs_errno_t oefs_initialize(fs_t** fs, fs_blkdev_t* dev);

#endif /* _OE_OEFS_H */
