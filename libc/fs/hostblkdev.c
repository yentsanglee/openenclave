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

#if 0
#define DUMP
#endif

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

static int _blkdev_get(fs_blkdev_t* dev, uint32_t blkno, fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;
    typedef oe_ocall_blkdev_get_args_t args_t;
    args_t* args = NULL;
    const uint16_t func = OE_OCALL_BLKDEV_GET;

#ifdef DUMP
    printf("HOST.GET{%u}\n", blkno);
#endif

    if (!device || !blk)
        goto done;

    if (!(args = fs_host_batch_calloc(device->batch, sizeof(args_t))))
        goto done;

    args->ret = -1;
    args->host_context = device->host_context;
    args->blkno = blkno;

    if (oe_ocall(func, (uint64_t)args, NULL) != OE_OK)
        goto done;

    if (args->ret != 0)
        goto done;

    memcpy(blk->data, args->blk, sizeof(args->blk));

    ret = 0;

done:

    if (args && device)
        fs_host_batch_free(device->batch);

    return ret;
}

static int _blkdev_put(
    fs_blkdev_t* dev,
    uint32_t blkno,
    const fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;
    typedef oe_ocall_blkdev_put_args_t args_t;
    args_t* args = NULL;
    const uint16_t func = OE_OCALL_BLKDEV_PUT;

#ifdef DUMP
    printf("HOST.PUT{%u}\n", blkno);
#endif

    if (!device || !blk)
        goto done;

    if (!(args = fs_host_batch_calloc(device->batch, sizeof(args_t))))
        goto done;

    args->ret = -1;
    args->host_context = device->host_context;
    args->blkno = blkno;
    memcpy(args->blk, blk->data, sizeof(args->blk));

    if (oe_ocall(func, (uint64_t)args, NULL) != OE_OK)
        goto done;

    if (args->ret != 0)
        goto done;

    ret = 0;

done:

    if (args && device)
        fs_host_batch_free(device->batch);

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

static int _blkdev_release(fs_blkdev_t* dev)
{
    int ret = -1;
    const uint16_t func = OE_OCALL_CLOSE_BLKDEV;
    blkdev_t* device = (blkdev_t*)dev;
    size_t new_ref_count;

    if (!device)
        goto done;

    pthread_spin_lock(&device->lock);
    new_ref_count = --device->ref_count;
    pthread_spin_unlock(&device->lock);

    if (new_ref_count == 0)
    {
        if (oe_ocall(func, (uint64_t)device->host_context, NULL) != OE_OK)
            goto done;

        fs_host_batch_delete(device->batch);

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
    blkdev_t* device = NULL;
    fs_host_batch_t* batch = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!device_name || !blkdev)
        goto done;

    if (!(name = oe_host_strndup(device_name, strlen(device_name))))
        goto done;

    if (!(device = calloc(1, sizeof(blkdev_t))))
        goto done;

    if (oe_ocall(func, (uint64_t)name, (uint64_t*)&host_context) != OE_OK)
        goto done;

    if (!host_context)
        goto done;

    if (!(batch = fs_host_batch_new(_get_batch_capacity())))
        goto done;

    device->base.get = _blkdev_get;
    device->base.put = _blkdev_put;
    device->base.add_ref = _blkdev_add_ref;
    device->base.release = _blkdev_release;
    device->batch = batch;
    device->ref_count = 1;
    device->host_context = host_context;

    *blkdev = &device->base;
    device = NULL;
    batch = NULL;

    ret = 0;

done:

    if (device)
        free(device);

    if (name)
        oe_host_free(name);

    if (batch)
        fs_host_batch_delete(batch);

    return ret;
}
