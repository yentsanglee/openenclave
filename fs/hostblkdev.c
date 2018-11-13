// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "hostblkdev.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atomic.h"
#include "blkdev.h"
#include "hostbatch.h"
#include "hostcalls.h"
#include "list.h"

#define MAX_NODES 16

static const fs_guid_t _GUID = FS_HOSTBLKDEV_GUID;

typedef struct _blkdev
{
    fs_blkdev_t base;
    volatile uint64_t ref_count;
    fs_host_batch_t* batch;
    void* handle;
} blkdev_t;

static size_t _get_batch_capacity()
{
    return sizeof(fs_hostblkdev_ocall_args_t);
}

static int _blkdev_get(fs_blkdev_t* d, uint32_t blkno, fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    typedef fs_hostblkdev_ocall_args_t args_t;
    args_t* args = NULL;

    if (!dev || !blk)
        goto done;

    if (!(args = fs_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->base.guid = _GUID;
    args->op = FS_HOSTBLKDEV_GET;
    args->get.ret = -1;
    args->get.handle = dev->handle;
    args->get.blkno = blkno;

    if (fs_host_call(&_GUID, &args->base) != 0)
        goto done;

    if (args->get.ret != 0)
        goto done;

    *blk = args->get.blk;

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
    typedef fs_hostblkdev_ocall_args_t args_t;
    args_t* args = NULL;

    if (!dev || !blk)
        goto done;

    if (!(args = fs_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->base.guid = _GUID;
    args->op = FS_HOSTBLKDEV_PUT;
    args->put.ret = -1;
    args->put.handle = dev->handle;
    args->put.blkno = blkno;
    args->put.blk = *blk;

    if (fs_host_call(&_GUID, &args->base) != 0)
        goto done;

    if (args->put.ret != 0)
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

    fs_atomic_increment(&dev->ref_count);

    ret = 0;

done:
    return ret;
}

static int _blkdev_release(fs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    typedef fs_hostblkdev_ocall_args_t args_t;
    args_t* args = NULL;

    if (!dev)
        goto done;

    if (fs_atomic_decrement(&dev->ref_count) == 0)
    {
        if (!(args = fs_host_batch_calloc(dev->batch, sizeof(args_t))))
            goto done;

        args->base.guid = _GUID;
        args->op = FS_HOSTBLKDEV_CLOSE;
        args->close.handle = dev->handle;

        if (fs_host_call(&_GUID, &args->base) != 0)
            goto done;

        fs_host_batch_delete(dev->batch);
        free(dev);
    }

    ret = 0;

done:

    return ret;
}

int fs_host_blkdev_open(fs_blkdev_t** blkdev, const char* path)
{
    int ret = -1;
    blkdev_t* dev = NULL;
    fs_host_batch_t* batch = NULL;
    typedef fs_hostblkdev_ocall_args_t args_t;
    args_t* args = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!blkdev || !path)
        goto done;

    if (!(dev = calloc(1, sizeof(blkdev_t))))
        goto done;

    if (!(batch = fs_host_batch_new(_get_batch_capacity())))
        goto done;

    if (!(args = fs_host_batch_calloc(batch, sizeof(args_t))))
        goto done;

    args->base.guid = _GUID;
    args->op = FS_HOSTBLKDEV_OPEN;

    if (!(args->open.path = fs_host_batch_strdup(batch, path)))
        goto done;

    if (fs_host_call(&_GUID, &args->base) != 0)
        goto done;

    if (!args->open.handle)
        goto done;

    dev->base.get = _blkdev_get;
    dev->base.put = _blkdev_put;
    dev->base.begin = _blkdev_begin;
    dev->base.end = _blkdev_end;
    dev->base.add_ref = _blkdev_add_ref;
    dev->base.release = _blkdev_release;
    dev->batch = batch;
    dev->ref_count = 1;
    dev->handle = args->open.handle;

    *blkdev = &dev->base;
    dev = NULL;
    batch = NULL;
    ret = 0;

done:

    if (args && dev && dev->batch)
        fs_host_batch_free(dev->batch);

    if (dev)
        free(dev);

    if (batch)
        fs_host_batch_delete(batch);

    return ret;
}
