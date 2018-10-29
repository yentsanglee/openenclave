// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _oe_oefs_h
#define _oe_oefs_h

#include <openenclave/internal/defs.h>
#include <openenclave/internal/types.h>
#include "blockdev.h"
#include "fs.h"

/* Compute required size of a file system with the given block count. */
oe_errno_t oefs_size(size_t nblocks, size_t* size);

/* Build an OE file system on the given device. */
oe_errno_t oefs_mkfs(oe_block_dev_t* dev, size_t nblocks);

/* Initialize the oefs instance from the given device. */
oe_errno_t oefs_initialize(fs_t** fs, oe_block_dev_t* dev);

#endif /* _oe_oefs_h */
