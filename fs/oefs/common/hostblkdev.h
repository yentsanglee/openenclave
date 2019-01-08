// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_HOSTBLKDEV_H
#define _FS_HOSTBLKDEV_H

#include "../enclave/common.h"
#include "../enclave/blk.h"

OE_EXTERNC_BEGIN

typedef enum _oefs_hostblkdev_op {
    OEFS_HOSTBLKDEV_OPEN,
    OEFS_HOSTBLKDEV_CLOSE,
    OEFS_HOSTBLKDEV_STAT,
    OEFS_HOSTBLKDEV_GET,
    OEFS_HOSTBLKDEV_PUT,
} oefs_hostblkdev_op_t;

typedef struct _oefs_oefs_ocall_args oefs_oefs_ocall_args_t;

struct _oefs_oefs_ocall_args
{
    oefs_oefs_ocall_args_t* next;
    oefs_hostblkdev_op_t op;
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
        oefs_blk_t blk;
    } get;
    struct
    {
        int ret;
        void* handle;
        struct
        {
            uint64_t total_size;
            uint64_t overhead_size;
            uint64_t usable_size;
        } buf;
    } stat;
    struct
    {
        int ret;
        void* handle;
        uint32_t blkno;
        oefs_blk_t blk;
    } put;
};

OE_EXTERNC_END

#endif /* _FS_HOSTBLKDEV_H */
