// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_HOSTBLKDEV_H
#define _FS_HOSTBLKDEV_H

#include "blkdev.h"
#include "args.h"

/* ac765314-ef16-4014-9c4e-7c9e2c2781da */
#define FS_HOSTBLKDEV_GUID                                 \
    {                                                      \
        0xac765314, 0xef16, 0x4014,                        \
        {                                                  \
            0x9c, 0x4e, 0x7c, 0x9e, 0x2c, 0x27, 0x81, 0xda \
        }                                                  \
    }

typedef enum _fs_hostblkdev_op {
    FS_HOSTBLKDEV_OPEN,
    FS_HOSTBLKDEV_CLOSE,
    FS_HOSTBLKDEV_GET,
    FS_HOSTBLKDEV_PUT,
} fs_hostblkdev_op_t;

typedef struct _fs_hostblkdev_ocall_args
{
    fs_args_t base;
    fs_hostblkdev_op_t op;
    struct
    {
        char* path;
        void* handle;
    } open;
    struct
    {
        void* handle;
    } close;
    struct
    {
        int ret;
        void* handle;
        uint32_t blkno;
        fs_blk_t blk;
    } get;
    struct
    {
        int ret;
        void* handle;
        uint32_t blkno;
        fs_blk_t blk;
    } put;
} fs_hostblkdev_ocall_args_t;

void fs_handle_hostblkdev_ocall(fs_hostblkdev_ocall_args_t* arg);

#endif /* _FS_HOSTBLKDEV_H */
