// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blockdev.h"
#include "hostbatch.h"

#if 0
#define DUMP
#endif

typedef struct _block_dev
{
    fs_block_dev_t base;
    size_t ref_count;
    pthread_spinlock_t lock;
    fs_host_batch_t* batch;
    void* host_context;
} block_dev_t;

static size_t _get_batch_capacity()
{
    size_t capacity = 0;

    if (sizeof(oe_ocall_block_dev_get_args_t) > capacity)
        capacity = sizeof(oe_ocall_block_dev_get_args_t);

    if (sizeof(oe_ocall_block_dev_put_args_t) > capacity)
        capacity = sizeof(oe_ocall_block_dev_put_args_t);

    return capacity;
}

static int _block_dev_get(fs_block_dev_t* dev, uint32_t blkno, void* data)
{
    int ret = -1;
    block_dev_t* device = (block_dev_t*)dev;
    typedef oe_ocall_block_dev_get_args_t args_t;
    args_t* args = NULL;
    const uint16_t func = OE_OCALL_BLOCK_DEVICE_GET;

#ifdef DUMP
    printf("HOST.GET{%u}\n", blkno);
#endif

    if (!device || !data)
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

    memcpy(data, args->block, sizeof(args->block));

    ret = 0;

done:

    if (args && device)
        fs_host_batch_free(device->batch);

    return ret;
}

static int _block_dev_put(fs_block_dev_t* dev, uint32_t blkno, const void* data)
{
    int ret = -1;
    block_dev_t* device = (block_dev_t*)dev;
    typedef oe_ocall_block_dev_put_args_t args_t;
    args_t* args = NULL;
    const uint16_t func = OE_OCALL_BLOCK_DEVICE_PUT;

#ifdef DUMP
    printf("HOST.PUT{%u}\n", blkno);
#endif

    if (!device || !data)
        goto done;

    if (!(args = fs_host_batch_calloc(device->batch, sizeof(args_t))))
        goto done;

    args->ret = -1;
    args->host_context = device->host_context;
    args->blkno = blkno;
    memcpy(args->block, data, sizeof(args->block));

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

static int _block_dev_release(fs_block_dev_t* dev)
{
    int ret = -1;
    const uint16_t func = OE_OCALL_CLOSE_BLOCK_DEVICE;
    block_dev_t* device = (block_dev_t*)dev;
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

int oe_open_host_block_dev(fs_block_dev_t** block_dev, const char* device_name)
{
    int ret = -1;
    char* name = NULL;
    void* host_context = NULL;
    const uint16_t func = OE_OCALL_OPEN_BLOCK_DEVICE;
    block_dev_t* device = NULL;
    fs_host_batch_t* batch = NULL;

    if (block_dev)
        *block_dev = NULL;

    if (!device_name || !block_dev)
        goto done;

    if (!(name = oe_host_strndup(device_name, strlen(device_name))))
        goto done;

    if (!(device = calloc(1, sizeof(block_dev_t))))
        goto done;

    if (oe_ocall(func, (uint64_t)name, (uint64_t*)&host_context) != OE_OK)
        goto done;

    if (!host_context)
        goto done;

    if (!(batch = fs_host_batch_new(_get_batch_capacity())))
        goto done;

    device->base.get = _block_dev_get;
    device->base.put = _block_dev_put;
    device->base.add_ref = _block_dev_add_ref;
    device->base.release = _block_dev_release;
    device->batch = batch;
    device->ref_count = 1;
    device->host_context = host_context;

    *block_dev = &device->base;
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
