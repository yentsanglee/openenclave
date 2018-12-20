// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../common/hostblkdev.h"
#include <openenclave/internal/calls.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../common/hostbatch.h"
#include "../common/hostblkdev.h"
#include "blkdev.h"
#include "common.h"
#include "list.h"

#define MAX_COUNT 8

typedef oefs_oefs_ocall_args_t args_t;

typedef struct _blkdev
{
    oefs_blkdev_t base;
    volatile uint64_t ref_count;
    oe_host_batch_t* batch;
    void* handle;

    /* Linked list of put requests. */
    args_t* head;
    args_t* tail;
    size_t count;
} blkdev_t;

static size_t _get_batch_capacity()
{
    return MAX_COUNT * sizeof(args_t);
}

static int _host_blkdev_flush(blkdev_t* dev)
{
    int ret = -1;

    if (dev->count)
    {
        if (oe_ocall(OE_OCALL_OEFS, (uint64_t)dev->head, NULL) != 0)
            goto done;

        if (dev->head->put.ret != 0)
        {
            dev->head = NULL;
            dev->tail = NULL;
            dev->count = 0;
            goto done;
        }

        dev->head = NULL;
        dev->tail = NULL;
        dev->count = 0;
    }

    ret = 0;

done:

    oe_host_batch_free(dev->batch);
    return ret;
}

static int _host_blkdev_stat(oefs_blkdev_t* d, oefs_blkdev_stat_t* stat)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    args_t* args = NULL;

    if (!dev || !stat)
        goto done;

    if (dev->count && _host_blkdev_flush(dev) != 0)
        goto done;

    if (!(args = oe_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->op = OEFS_HOSTBLKDEV_STAT;
    args->stat.ret = -1;
    args->stat.handle = dev->handle;

    if (oe_ocall(OE_OCALL_OEFS, (uint64_t)args, NULL) != 0)
        goto done;

    if (args->stat.ret != 0)
        goto done;

    stat->total_size = args->stat.buf.total_size;
    stat->overhead_size = args->stat.buf.overhead_size;
    stat->usable_size = args->stat.buf.usable_size;

    ret = 0;

done:

    if (args && dev)
        oe_host_batch_free(dev->batch);

    return ret;
}

static int _host_blkdev_get(oefs_blkdev_t* d, uint32_t blkno, oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    args_t* args = NULL;

    if (!dev || !blk)
        goto done;

    if (dev->count && _host_blkdev_flush(dev) != 0)
        goto done;

    if (!(args = oe_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->op = OEFS_HOSTBLKDEV_GET;
    args->get.ret = -1;
    args->get.handle = dev->handle;
    args->get.blkno = blkno;

    if (oe_ocall(OE_OCALL_OEFS, (uint64_t)args, NULL) != 0)
        goto done;

    if (args->get.ret != 0)
        goto done;

    oefs_blk_copy(blk, &args->get.blk);

    ret = 0;

done:

    if (args && dev)
        oe_host_batch_free(dev->batch);

    return ret;
}

static int _host_blkdev_put(
    oefs_blkdev_t* d,
    uint32_t blkno,
    const oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    args_t* args = NULL;

    if (!dev || !blk)
        goto done;

    if (dev->count == MAX_COUNT)
    {
        if (_host_blkdev_flush(dev) != 0)
            goto done;
    }

    if (!(args = oe_host_batch_calloc(dev->batch, sizeof(args_t))))
        goto done;

    args->op = OEFS_HOSTBLKDEV_PUT;
    args->put.ret = -1;
    args->put.handle = dev->handle;
    args->put.blkno = blkno;
    args->put.blk = *blk;

    /* Append new args to list. */
    {
        if (dev->tail)
            dev->tail->next = args;
        else
            dev->head = args;

        dev->tail = args;
        dev->count++;
    }

    ret = 0;

done:

    return ret;
}

static int _host_blkdev_begin(oefs_blkdev_t* d)
{
    return 0;
}

static int _host_blkdev_end(oefs_blkdev_t* d)
{
    blkdev_t* dev = (blkdev_t*)d;

    if (dev->count)
        return _host_blkdev_flush(dev);

    return 0;
}

static int _host_blkdev_add_ref(oefs_blkdev_t* d)
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

static int _host_blkdev_release(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;
    args_t* args = NULL;

    if (!dev)
        goto done;

    if (oe_atomic_decrement(&dev->ref_count) == 0)
    {
        if (!(args = oe_host_batch_calloc(dev->batch, sizeof(args_t))))
            goto done;

        args->op = OEFS_HOSTBLKDEV_CLOSE;
        args->close.handle = dev->handle;

        if (oe_ocall(OE_OCALL_OEFS, (uint64_t)args, NULL) != 0)
            goto done;

        oe_host_batch_delete(dev->batch);
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
    oe_host_batch_t* batch = NULL;
    args_t* args = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!blkdev || !path)
        goto done;

    if (!(dev = calloc(1, sizeof(blkdev_t))))
        goto done;

    if (!(batch = oe_host_batch_new(_get_batch_capacity())))
        goto done;

    if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
        goto done;

    args->op = OEFS_HOSTBLKDEV_OPEN;

    if (!(args->open.path = oe_host_batch_strdup(batch, path)))
        goto done;

    if (oe_ocall(OE_OCALL_OEFS, (uint64_t)args, NULL) != 0)
        goto done;

    if (!args->open.handle)
        goto done;

    dev->base.stat = _host_blkdev_stat;
    dev->base.get = _host_blkdev_get;
    dev->base.put = _host_blkdev_put;
    dev->base.begin = _host_blkdev_begin;
    dev->base.end = _host_blkdev_end;
    dev->base.add_ref = _host_blkdev_add_ref;
    dev->base.release = _host_blkdev_release;
    dev->batch = batch;
    dev->ref_count = 1;
    dev->handle = args->open.handle;

    *blkdev = &dev->base;
    dev = NULL;
    batch = NULL;
    ret = 0;

done:

    if (args && dev && dev->batch)
        oe_host_batch_free(dev->batch);

    if (dev)
        free(dev);

    if (batch)
        oe_host_batch_delete(batch);

    return ret;
}
