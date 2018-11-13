// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_BLKDEV_H
#define _FS_BLKDEV_H

#include <stdint.h>
#include "common.h"

FS_EXTERN_C_BEGIN

#define FS_KEY_SIZE 32

typedef struct _fs_blk
{
    uint8_t data[FS_BLOCK_SIZE];
} fs_blk_t;

typedef struct _fs_blkdev fs_blkdev_t;

struct _fs_blkdev
{
    int (*get)(fs_blkdev_t* dev, uint32_t blkno, fs_blk_t* blk);

    int (*put)(fs_blkdev_t* dev, uint32_t blkno, const fs_blk_t* blk);

    int (*begin)(fs_blkdev_t* dev);

    int (*end)(fs_blkdev_t* dev);

    int (*add_ref)(fs_blkdev_t* dev);

    int (*release)(fs_blkdev_t* dev);
};

int fs_host_blkdev_open(fs_blkdev_t** blkdev, const char* path);

int fs_ram_blkdev_open(fs_blkdev_t** blkdev, size_t size);

int fs_cache_blkdev_open(fs_blkdev_t** blkdev, fs_blkdev_t* next);

int fs_crypto_blkdev_open(
    fs_blkdev_t** blkdev,
    const uint8_t key[FS_KEY_SIZE],
    fs_blkdev_t* next);

int fs_auth_crypto_blkdev_open(
    fs_blkdev_t** blkdev,
    bool initialize,
    size_t nblks,
    const uint8_t key[FS_KEY_SIZE],
    fs_blkdev_t* next);

/* Get the number of extra blocks needed by a Merkle block device. */
int fs_merkle_blkdev_get_extra_blocks(size_t nblks, size_t* extra_nblks);

int fs_merkle_blkdev_open(
    fs_blkdev_t** blkdev,
    bool initialize,
    size_t nblks,
    fs_blkdev_t* next);

FS_EXTERN_C_END

#endif /* _FS_BLKDEV_H */
