// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_BLKDEV_H
#define _OEFS_BLKDEV_H

#include <stdint.h>
#include "common.h"

OE_EXTERNC_BEGIN

typedef struct _oefs_blkdev oefs_blkdev_t;

typedef struct _oefs_blkdev_stat oefs_blkdev_stat_t;

struct _oefs_blkdev_stat
{
    /* The total size of the device in bytes. */
    uint64_t total_size;

    /* The number of bytes dedicated to overhead (e.g., metadata). */
    uint64_t overhead_size;

    /* The number of usable bytes (total_size - overhead_size) */
    uint64_t usable_size;
};

struct _oefs_blkdev
{
    int (*stat)(oefs_blkdev_t* dev, oefs_blkdev_stat_t* stat);

    int (*get)(oefs_blkdev_t* dev, uint32_t blkno, oefs_blk_t* blk);

    int (*put)(oefs_blkdev_t* dev, uint32_t blkno, const oefs_blk_t* blk);

    int (*begin)(oefs_blkdev_t* dev);

    int (*end)(oefs_blkdev_t* dev);

    int (*add_ref)(oefs_blkdev_t* dev);

    int (*release)(oefs_blkdev_t* dev);
};

int oefs_host_blkdev_open(oefs_blkdev_t** blkdev, const char* path);

int oefs_ram_blkdev_open(oefs_blkdev_t** blkdev, size_t size);

int oefs_cache_blkdev_open(oefs_blkdev_t** blkdev, oefs_blkdev_t* next);

int oefs_crypto_blkdev_open(
    oefs_blkdev_t** blkdev,
    const uint8_t key[OEFS_KEY_SIZE],
    oefs_blkdev_t* next);

/* Get the number of extra blocks needed by a Merkle block device. */
int oefs_merkle_blkdev_get_extra_blocks(size_t nblks, size_t* extra_nblks);

/* Get the number of extra blocks needed by a auth-Merkle block device. */
int oefs_merkle2_blkdev_get_extra_blocks(size_t nblks, size_t* extra_nblks);

int oefs_merkle_blkdev_open(
    oefs_blkdev_t** blkdev,
    bool initialize,
    size_t nblks,
    oefs_blkdev_t* next);

int oefs_merkle2_blkdev_open(
    oefs_blkdev_t** blkdev,
    bool initialize,
    size_t nblks,
    const uint8_t key[OEFS_KEY_SIZE],
    oefs_blkdev_t* next);

OE_EXTERNC_END

#endif /* _OEFS_BLKDEV_H */
