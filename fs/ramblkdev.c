// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blkdev.h"
#include "atomic.h"

typedef struct _blkdev
{
    fs_blkdev_t base;
    volatile uint64_t ref_count;
    uint8_t* mem;
    size_t size;
} blkdev_t;

static int _blkdev_release(fs_blkdev_t* dev)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device)
        goto done;

    if (fs_atomic_decrement(&device->ref_count) == 0)
    {
        free(device->mem);
        free(device);
    }

    ret = 0;

done:
    return ret;
}

static int _blkdev_get(fs_blkdev_t* dev, uint32_t blkno, fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device || !blk)
        goto done;

    uint8_t* ptr = device->mem + (blkno * FS_BLOCK_SIZE);

    if (ptr + FS_BLOCK_SIZE > device->mem + device->size)
        goto done;

    memcpy(blk->data, ptr, FS_BLOCK_SIZE);

    ret = 0;

done:

    return ret;
}

static int _blkdev_put(fs_blkdev_t* dev, uint32_t blkno, const fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device || !blk)
        goto done;

    uint8_t* ptr = device->mem + (blkno * FS_BLOCK_SIZE);

    if (ptr + FS_BLOCK_SIZE > device->mem + device->size)
        goto done;

    memcpy(ptr, blk->data, FS_BLOCK_SIZE);

    ret = 0;

done:

    return ret;
}

static int _blkdev_begin(fs_blkdev_t* d)
{
    return 0;
}

static int _blkdev_end(fs_blkdev_t* d)
{
    return 0;
}

static int _blkdev_add_ref(fs_blkdev_t* dev)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device)
        goto done;

    fs_atomic_increment(&device->ref_count);

    ret = 0;

done:
    return ret;
}
int fs_ram_blkdev_open(fs_blkdev_t** blkdev, size_t size)
{
    int ret = -1;
    blkdev_t* device = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!size || !blkdev)
        goto done;

    /* Size must be a multiple of the block size. */
    if (size % FS_BLOCK_SIZE)
        goto done;

    if (!(device = calloc(1, sizeof(blkdev_t))))
        goto done;

    device->base.get = _blkdev_get;
    device->base.put = _blkdev_put;
    device->base.begin = _blkdev_begin;
    device->base.end = _blkdev_end;
    device->base.add_ref = _blkdev_add_ref;
    device->base.release = _blkdev_release;
    device->ref_count = 1;
    device->size = size;

    if (!(device->mem = calloc(1, size)))
        goto done;

    *blkdev = &device->base;
    device = NULL;

    ret = 0;

done:

    if (device)
        free(device);

    return ret;
}
