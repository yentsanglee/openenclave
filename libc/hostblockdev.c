#include "blockdev.h"
#include <openenclave/internal/calls.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _host_block_dev
{
    oe_block_dev_t base;
    void* host_context;
} host_block_dev_t;

static int _host_block_dev_close(oe_block_dev_t* dev)
{
    int ret = -1;
    const uint16_t func = OE_OCALL_CLOSE_BLOCK_DEVICE;
    host_block_dev_t* device = (host_block_dev_t*)dev;

    if (!device)
        goto done;

    if (oe_ocall(func, (uint64_t)device->host_context, NULL) != OE_OK)
        goto done;

    free(dev);

    ret = 0;

done:
    return ret;
}

static int _host_block_dev_get(
    oe_block_dev_t* dev,
    uint32_t blkno,
    void* data)
{
    int ret = -1;
    host_block_dev_t* device = (host_block_dev_t*)dev;
    typedef oe_ocall_block_dev_get_args_t args_t;
    args_t* args = NULL;
    const uint16_t func = OE_OCALL_BLOCK_DEVICE_GET;

    if (!device || !data)
        goto done;

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
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

    if (args)
        oe_host_free(args);

    return ret;
}

static int _host_block_dev_put(
    oe_block_dev_t* dev,
    uint32_t blkno,
    const void* data)
{
    int ret = -1;
    host_block_dev_t* device = (host_block_dev_t*)dev;
    typedef oe_ocall_block_dev_put_args_t args_t;
    args_t* args = NULL;
    const uint16_t func = OE_OCALL_BLOCK_DEVICE_PUT;

    if (!device || !data)
        goto done;

    if (!(args = oe_host_calloc(1, sizeof(args_t))))
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

    if (args)
        oe_host_free(args);

    return ret;
}

oe_result_t oe_open_host_block_dev(
    const char* device_name,
    oe_block_dev_t** block_dev)
{
    oe_result_t result = OE_FAILURE;
    char* name = NULL;
    void* host_context = NULL;
    const uint16_t func = OE_OCALL_OPEN_BLOCK_DEVICE;
    host_block_dev_t* device = NULL;

    if (block_dev)
        *block_dev = NULL;

    if (!device_name || !block_dev)
        goto done;

    if (!(name = oe_host_strndup(device_name, strlen(device_name))))
        goto done;

    if (!(device = calloc(1, sizeof(host_block_dev_t))))
        goto done;

    if (oe_ocall(func, (uint64_t)name, (uint64_t*)&host_context) != OE_OK)
        goto done;

    if (!host_context)
        goto done;

    device->base.close = _host_block_dev_close;
    device->base.get = _host_block_dev_get;
    device->base.put = _host_block_dev_put;
    device->host_context = host_context;

    *block_dev = &device->base;
    device = NULL;

    result = OE_OK;

done:

    if (device)
        free(device);

    if (name)
        oe_host_free(name);

    return result;
}
