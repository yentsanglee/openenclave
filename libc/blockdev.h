// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _oe_blockdev_h
#define _oe_blockdev_h

#include <openenclave/enclave.h>
#include <stdint.h>

typedef struct _oe_block_dev oe_block_dev_t;

struct _oe_block_dev
{
    int (*close)(oe_block_dev_t* dev);

    int (*get)(oe_block_dev_t* dev, uint32_t blkno, void* data);

    int (*put)(oe_block_dev_t* dev, uint32_t blkno, const void* data);
};

/* Ask host to open a block device. */
oe_result_t oe_open_host_block_dev(
    const char* device_name,
    oe_block_dev_t** block_dev);

oe_result_t oe_open_ram_block_dev(size_t size, oe_block_dev_t** block_dev);

#endif /* _oe_blockdev_h */
