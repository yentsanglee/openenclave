// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_OEFS_H
#define _FS_OEFS_H

#include "blkdev.h"
#include "common.h"
#include "fs.h"

/* Compute required size of a file system with the given block count. */
fs_errno_t oefs_size(size_t nblks, size_t* size);

/* Build an OE file system on the given device. */
fs_errno_t oefs_mkfs(fs_blkdev_t* dev, size_t nblks);

#endif /* _FS_OEFS_H */
