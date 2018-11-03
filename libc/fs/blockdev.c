// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "blockdev.h"
#include <string.h>
#include "common.h"

int fs_block_dev_read(
    fs_block_dev_t* dev,
    size_t blkno,
    void* data,
    size_t size)
{
    int rc = -1;
    size_t nblocks;
    size_t i;
    uint8_t* ptr;
    size_t rem;

    if (!dev || !data)
        goto done;

    nblocks = (size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    ptr = (uint8_t*)data;
    rem = size;

    for (i = blkno; rem > 0 && i < blkno + nblocks; i++)
    {
        uint8_t block[FS_BLOCK_SIZE];
        size_t n;

        if (dev->get(dev, i, block) != 0)
            goto done;

        n = rem < FS_BLOCK_SIZE ? rem : FS_BLOCK_SIZE;

        memcpy(ptr, block, n);
        ptr += n;
        rem -= n;
    }

    rc = 0;

done:
    return rc;
}

int fs_block_dev_write(
    fs_block_dev_t* dev,
    size_t blkno,
    const void* data,
    size_t size)
{
    int rc = -1;
    size_t nblocks;
    size_t i;
    const uint8_t* ptr;
    size_t rem;

    if (!dev || !data)
        goto done;

    nblocks = (size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    ptr = (uint8_t*)data;
    rem = size;

    for (i = blkno; rem > 0 && i < blkno + nblocks; i++)
    {
        uint8_t block[FS_BLOCK_SIZE];
        size_t n;

        /* Read the existing block */
        if (dev->get(dev, i, block) != 0)
            goto done;

        /* Update this block */
        n = rem < FS_BLOCK_SIZE ? rem : FS_BLOCK_SIZE;
        memcpy(block, ptr, n);

        /* Rewrite the block */
        if (dev->put(dev, i, block) != 0)
            goto done;

        ptr += n;
        rem -= n;
    }

    rc = 0;

done:
    return rc;
}
