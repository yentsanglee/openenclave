// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_HOSTBLKDEV_H
#define _FS_HOSTBLKDEV_H

#include "../enclave/common.h"

OE_EXTERNC_BEGIN

typedef enum _oefs_hostblkdev_op {
    OEFS_HOSTBLKDEV_OPEN,
    OEFS_HOSTBLKDEV_CLOSE,
    OEFS_HOSTBLKDEV_GET,
    OEFS_HOSTBLKDEV_PUT,
} oefs_hostblkdev_op_t;

typedef struct _oefs_hostblkdev_ocall_args
{
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
        oe_blk_t blk;
    } get;
    struct
    {
        int ret;
        void* handle;
        uint32_t blkno;
        oe_blk_t blk;
    } put;
} oefs_hostblkdev_ocall_args_t;

void oefs_handle_hostblkdev_ocall(oefs_hostblkdev_ocall_args_t* arg);

OE_EXTERNC_END

#endif /* _FS_HOSTBLKDEV_H */
