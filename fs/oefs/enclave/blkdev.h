// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_BLKDEV_H
#define _OEFS_BLKDEV_H

#include <stdint.h>
#include "common.h"

OE_EXTERNC_BEGIN

typedef struct _oe_blkdev oe_blkdev_t;

struct _oe_blkdev
{
    int (*get)(oe_blkdev_t* dev, uint32_t blkno, oe_blk_t* blk);

    int (*put)(oe_blkdev_t* dev, uint32_t blkno, const oe_blk_t* blk);

    int (*begin)(oe_blkdev_t* dev);

    int (*end)(oe_blkdev_t* dev);

    int (*add_ref)(oe_blkdev_t* dev);

    int (*release)(oe_blkdev_t* dev);
};

int oefs_host_blkdev_open(oe_blkdev_t** blkdev, const char* path);

int oefs_ram_blkdev_open(oe_blkdev_t** blkdev, size_t size);

int oefs_cache_blkdev_open(oe_blkdev_t** blkdev, oe_blkdev_t* next);

int oefs_crypto_blkdev_open(
    oe_blkdev_t** blkdev,
    const uint8_t key[OEFS_KEY_SIZE],
    oe_blkdev_t* next);

int oefs_auth_crypto_blkdev_open(
    oe_blkdev_t** blkdev,
    bool initialize,
    size_t nblks,
    const uint8_t key[OEFS_KEY_SIZE],
    oe_blkdev_t* next);

/* Get the number of extra blocks needed by a Merkle block device. */
int oefs_merkle_blkdev_get_extra_blocks(size_t nblks, size_t* extra_nblks);

int oefs_merkle_blkdev_open(
    oe_blkdev_t** blkdev,
    bool initialize,
    size_t nblks,
    oe_blkdev_t* next);

OE_EXTERNC_END

#endif /* _OEFS_BLKDEV_H */
