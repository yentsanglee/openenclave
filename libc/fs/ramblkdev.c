// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blkdev.h"

#define BLOCK_SIZE 512

typedef struct _blkdev
{
    fs_blkdev_t base;
    size_t ref_count;
    pthread_spinlock_t lock;
    uint8_t* mem;
    size_t size;
} blkdev_t;

static int _blkdev_release(fs_blkdev_t* dev)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;
    size_t new_ref_count;

    if (!device)
        goto done;

    pthread_spin_lock(&device->lock);
    new_ref_count = --device->ref_count;
    pthread_spin_unlock(&device->lock);

    if (new_ref_count == 0)
    {
        free(device->mem);
        free(device);
    }

    ret = 0;

done:
    return ret;
}

static int _blkdev_get(fs_blkdev_t* dev, uint32_t blkno, fs_block_t* block)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device || !block)
        goto done;

    uint8_t* ptr = device->mem + (blkno * BLOCK_SIZE);

    if (ptr + BLOCK_SIZE > device->mem + device->size)
        goto done;

    memcpy(block->data, ptr, BLOCK_SIZE);

    ret = 0;

done:

    return ret;
}

static int _blkdev_put(fs_blkdev_t* dev, uint32_t blkno, const fs_block_t* block)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device || !block)
        goto done;

    uint8_t* ptr = device->mem + (blkno * BLOCK_SIZE);

    if (ptr + BLOCK_SIZE > device->mem + device->size)
        goto done;

    memcpy(ptr, block->data, BLOCK_SIZE);

    ret = 0;

done:

    return ret;
}

static int _blkdev_add_ref(fs_blkdev_t* dev)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device)
        goto done;

    pthread_spin_lock(&device->lock);
    device->ref_count++;
    pthread_spin_unlock(&device->lock);

    ret = 0;

done:
    return ret;
}
int fs_open_ram_blkdev(fs_blkdev_t** blkdev, size_t size)
{
    int ret = -1;
    blkdev_t* device = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!size || !blkdev)
        goto done;

    /* Size must be a multiple of the block size. */
    if (size % BLOCK_SIZE)
        goto done;

    if (!(device = calloc(1, sizeof(blkdev_t))))
        goto done;

    device->base.get = _blkdev_get;
    device->base.put = _blkdev_put;
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
