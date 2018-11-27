// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../common/hostblkdev.h"
#include <openenclave/internal/calls.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/hostblkdev.h"
#include "blkdev.h"
#include "common.h"
#include "hostbatch.h"
#include "list.h"

#define MAX_NODES 16

typedef struct _blkdev
{
    oefs_blkdev_t base;
    volatile uint64_t ref_count;
    oefs_host_batch_t* batch;
    void* handle;
} blkdev_t;

static size_t _get_batch_capacity()
{
    return sizeof(oefs_oefs_ocall_args_t);
}

static int _blkdev_get(oefs_blkdev_t* d, uint32_t blkno, oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    typedef oefs_oefs_ocall_args_t args_t;
    args_t* args = NULL;

    if (!dev || !blk)
        goto done;

    if (!(args = oefs_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->op = OEFS_HOSTBLKDEV_GET;
    args->get.ret = -1;
    args->get.handle = dev->handle;
    args->get.blkno = blkno;

    if (oe_ocall(OE_OCALL_OEFS, (uint64_t)args, NULL) != 0)
        goto done;

    if (args->get.ret != 0)
        goto done;

    *blk = args->get.blk;

    ret = 0;

done:

    if (args && dev)
        oefs_host_batch_free(dev->batch);

    return ret;
}

static int _blkdev_put(oefs_blkdev_t* d, uint32_t blkno, const oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    typedef oefs_oefs_ocall_args_t args_t;
    args_t* args = NULL;

    if (!dev || !blk)
        goto done;

    if (!(args = oefs_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->op = OEFS_HOSTBLKDEV_PUT;
    args->put.ret = -1;
    args->put.handle = dev->handle;
    args->put.blkno = blkno;
    args->put.blk = *blk;

    if (oe_ocall(OE_OCALL_OEFS, (uint64_t)args, NULL) != 0)
        goto done;

    if (args->put.ret != 0)
        goto done;

    ret = 0;

done:

    if (args && dev)
        oefs_host_batch_free(dev->batch);

    return ret;
}

static int _blkdev_begin(oefs_blkdev_t* d)
{
    return 0;
}

static int _blkdev_end(oefs_blkdev_t* d)
{
    return 0;
}

static int _blkdev_add_ref(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev)
        goto done;

    oe_atomic_increment(&dev->ref_count);

    ret = 0;

done:
    return ret;
}

static int _blkdev_release(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    typedef oefs_oefs_ocall_args_t args_t;
    args_t* args = NULL;

    if (!dev)
        goto done;

    if (oe_atomic_decrement(&dev->ref_count) == 0)
    {
        if (!(args = oefs_host_batch_calloc(dev->batch, sizeof(args_t))))
            goto done;

        args->op = OEFS_HOSTBLKDEV_CLOSE;
        args->close.handle = dev->handle;

        if (oe_ocall(OE_OCALL_OEFS, (uint64_t)args, NULL) != 0)
            goto done;

        oefs_host_batch_delete(dev->batch);
        free(dev);
    }

    ret = 0;

done:

    return ret;
}

int oefs_host_blkdev_open(oefs_blkdev_t** blkdev, const char* path)
{
    int ret = -1;
    blkdev_t* dev = NULL;
    oefs_host_batch_t* batch = NULL;
    typedef oefs_oefs_ocall_args_t args_t;
    args_t* args = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!blkdev || !path)
        goto done;

    if (!(dev = calloc(1, sizeof(blkdev_t))))
        goto done;

    if (!(batch = oefs_host_batch_new(_get_batch_capacity())))
        goto done;

    if (!(args = oefs_host_batch_calloc(batch, sizeof(args_t))))
        goto done;

    args->op = OEFS_HOSTBLKDEV_OPEN;

    if (!(args->open.path = oefs_host_batch_strdup(batch, path)))
        goto done;

    if (oe_ocall(OE_OCALL_OEFS, (uint64_t)args, NULL) != 0)
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
        oefs_host_batch_free(dev->batch);

    if (dev)
        free(dev);

    if (batch)
        oefs_host_batch_delete(batch);

    return ret;
}
