// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blkdev.h"
#include "hostbatch.h"
#include "list.h"

#define MAX_NODES 16

typedef struct _blkdev
{
    fs_blkdev_t base;
    size_t ref_count;
    pthread_spinlock_t lock;
    fs_host_batch_t* batch;
    void* host_context;
} blkdev_t;

static size_t _get_batch_capacity()
{
    size_t capacity = 0;

    if (sizeof(oe_ocall_blkdev_get_args_t) > capacity)
        capacity = sizeof(oe_ocall_blkdev_get_args_t);

    if (sizeof(oe_ocall_blkdev_put_args_t) > capacity)
        capacity = sizeof(oe_ocall_blkdev_put_args_t);

    return capacity;
}

static int _blkdev_get(fs_blkdev_t* d, uint32_t blkno, fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    typedef oe_ocall_blkdev_get_args_t args_t;
    args_t* args = NULL;

    if (!dev || !blk)
        goto done;

    if (!(args = fs_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->ret = -1;
    args->host_context = dev->host_context;
    args->blkno = blkno;

    if (oe_ocall(OE_OCALL_BLKDEV_GET, (uint64_t)args, NULL) != OE_OK)
        goto done;

    if (args->ret != 0)
        goto done;

    memcpy(blk->data, args->blk, sizeof(args->blk));

    ret = 0;

done:

    if (args && dev)
        fs_host_batch_free(dev->batch);

    return ret;
}

static int _blkdev_put(fs_blkdev_t* d, uint32_t blkno, const fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    typedef oe_ocall_blkdev_put_args_t args_t;
    args_t* args = NULL;
    const uint16_t func = OE_OCALL_BLKDEV_PUT;

    if (!dev || !blk)
        goto done;

    if (!(args = fs_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->ret = -1;
    args->host_context = dev->host_context;
    args->blkno = blkno;
    memcpy(args->blk, blk->data, sizeof(args->blk));

    if (oe_ocall(func, (uint64_t)args, NULL) != OE_OK)
        goto done;

    if (args->ret != 0)
        goto done;

    ret = 0;

done:

    if (args && dev)
        fs_host_batch_free(dev->batch);

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

static int _blkdev_add_ref(fs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev)
        goto done;

    pthread_spin_lock(&dev->lock);
    dev->ref_count++;
    pthread_spin_unlock(&dev->lock);

    ret = 0;

done:
    return ret;
}

static int _blkdev_release(fs_blkdev_t* d)
{
    int ret = -1;
    const uint16_t func = OE_OCALL_CLOSE_BLKDEV;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev)
        goto done;

    pthread_spin_lock(&dev->lock);
    pthread_spin_unlock(&dev->lock);

    if (--dev->ref_count == 0)
    {
        if (oe_ocall(func, (uint64_t)dev->host_context, NULL) != OE_OK)
            goto done;

        fs_host_batch_delete(dev->batch);
        free(dev);
    }

    ret = 0;

done:
    return ret;
}

int fs_open_host_blkdev(fs_blkdev_t** blkdev, const char* device_name)
{
    int ret = -1;
    char* name = NULL;
    void* host_context = NULL;
    const uint16_t func = OE_OCALL_OPEN_BLKDEV;
    blkdev_t* dev = NULL;
    fs_host_batch_t* batch = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!device_name || !blkdev)
        goto done;

    if (!(name = oe_host_strndup(device_name, strlen(device_name))))
        goto done;

    if (!(dev = calloc(1, sizeof(blkdev_t))))
        goto done;

    if (oe_ocall(func, (uint64_t)name, (uint64_t*)&host_context) != OE_OK)
        goto done;

    if (!host_context)
        goto done;

    if (!(batch = fs_host_batch_new(_get_batch_capacity())))
        goto done;

    dev->base.get = _blkdev_get;
    dev->base.put = _blkdev_put;
    dev->base.begin = _blkdev_begin;
    dev->base.end = _blkdev_end;
    dev->base.add_ref = _blkdev_add_ref;
    dev->base.release = _blkdev_release;
    dev->batch = batch;
    dev->ref_count = 1;
    dev->host_context = host_context;

    *blkdev = &dev->base;
    dev = NULL;
    batch = NULL;

    ret = 0;

done:

    if (dev)
        free(dev);

    if (name)
        oe_host_free(name);

    if (batch)
        fs_host_batch_delete(batch);

    return ret;
}
