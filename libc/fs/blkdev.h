// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_BLKDEV_H
#define _FS_BLKDEV_H

#include <stdint.h>
#include "common.h"

#define FS_KEY_SIZE 32

typedef struct _fs_block
{
    char data[FS_BLOCK_SIZE];
}
fs_block_t;

typedef struct _fs_blkdev fs_blkdev_t;

struct _fs_blkdev
{
    int (*get)(fs_blkdev_t* dev, uint32_t blkno, fs_block_t* block);

    int (*put)(fs_blkdev_t* dev, uint32_t blkno, const fs_block_t* block);

    int (*add_ref)(fs_blkdev_t* dev);

    int (*release)(fs_blkdev_t* dev);
};

int fs_open_host_blkdev(fs_blkdev_t** blkdev, const char* device_name);

int fs_open_ram_blkdev(fs_blkdev_t** blkdev, size_t size);

int fs_open_cache_blkdev(fs_blkdev_t** blkdev, fs_blkdev_t* next);

int fs_open_crypto_blkdev(
    fs_blkdev_t** blkdev,
    const uint8_t key[FS_KEY_SIZE],
    fs_blkdev_t* next);

#endif /* _FS_BLKDEV_H */
