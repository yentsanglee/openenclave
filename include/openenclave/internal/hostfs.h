// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_HOSTFS_H
#define _OE_HOSTFS_H

typedef enum _oe_hostfs_op
{
    OE_HOSTFS_OPEN,
    OE_HOSTFS_LSEEK,
    OE_HOSTFS_READ,
    OE_HOSTFS_WRITE,
    OE_HOSTFS_CLOSE,
    __OE_HOSTFS_MAX = OE_ENUM_MAX,
}
oe_hostfs_op_t;

typedef struct _oe_hostfs_args
{
    long op;
    long ret;
    long err; /* errno value */

    union {
        struct
        {
            long arg1;
            long arg2;
            long arg3;
            long arg4;
            long arg5;
            long arg6;
        } generic;
        struct
        {
            long fd;
            long offset;
            long whence;
        } lseek;
        struct
        {
            const char* pathname;
            long flags;
            long mode;
        } open;
        struct
        {
            long fd;
            void* buf;
            long count;
        } read;
        struct
        {
            long fd;
            void* buf;
            long count;
        } write;
        struct
        {
            long fd;
        } close;
    } u;
} oe_hostfs_args_t;

#endif /* _OE_HOSTFS_H */
