// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockdev.h"

#define BLOCK_SIZE 512

typedef struct _block_dev
{
    fs_block_dev_t base;
    size_t ref_count;
    pthread_spinlock_t lock;
    uint8_t* mem;
    size_t size;
} block_dev_t;

static int _block_dev_release(fs_block_dev_t* dev)
{
    int ret = -1;
    block_dev_t* device = (block_dev_t*)dev;
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

static int _block_dev_get(fs_block_dev_t* dev, uint32_t blkno, fs_block_t* block)
{
    int ret = -1;
    block_dev_t* device = (block_dev_t*)dev;

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

static int _block_dev_put(fs_block_dev_t* dev, uint32_t blkno, const fs_block_t* block)
{
    int ret = -1;
    block_dev_t* device = (block_dev_t*)dev;

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

static int _block_dev_add_ref(fs_block_dev_t* dev)
{
    int ret = -1;
    block_dev_t* device = (block_dev_t*)dev;

    if (!device)
        goto done;

    pthread_spin_lock(&device->lock);
    device->ref_count++;
    pthread_spin_unlock(&device->lock);

    ret = 0;

done:
    return ret;
}
int fs_open_ram_block_dev(fs_block_dev_t** block_dev, size_t size)
{
    int ret = -1;
    block_dev_t* device = NULL;

    if (block_dev)
        *block_dev = NULL;

    if (!size || !block_dev)
        goto done;

    /* Size must be a multiple of the block size. */
    if (size % BLOCK_SIZE)
        goto done;

    if (!(device = calloc(1, sizeof(block_dev_t))))
        goto done;

    device->base.get = _block_dev_get;
    device->base.put = _block_dev_put;
    device->base.add_ref = _block_dev_add_ref;
    device->base.release = _block_dev_release;
    device->ref_count = 1;
    device->size = size;

    if (!(device->mem = calloc(1, size)))
        goto done;

    *block_dev = &device->base;
    device = NULL;

    ret = 0;

done:

    if (device)
        free(device);

    return ret;
}
