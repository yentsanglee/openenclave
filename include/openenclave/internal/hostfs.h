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
    OE_HOSTFS_OPENDIR,
    OE_HOSTFS_READDIR,
    OE_HOSTFS_CLOSEDIR,
    __OE_HOSTFS_MAX = OE_ENUM_MAX,
}
oe_hostfs_op_t;

typedef struct _oe_hostfs_args
{
    long op;
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
            long ret;
            const char* pathname;
            long flags;
            long mode;
        } open;
        struct
        {
            long ret;
            long fd;
            long offset;
            long whence;
        } lseek;
        struct
        {
            long ret;
            long fd;
            void* buf;
            long count;
        } read;
        struct
        {
            long ret;
            long fd;
            void* buf;
            long count;
        } write;
        struct
        {
            long ret;
            long fd;
        } close;
        struct
        {
            const char* name;
            void* dir;
        } opendir;
        struct
        {
            struct
            {
                unsigned long d_ino;
                unsigned long d_off;
                unsigned short d_reclen;
                unsigned char d_type;
                unsigned char __d_reserved;
                char d_name[256];
            }
            buf;
            void* entry;
            void* dir;
        } readdir;
        struct
        {
            long ret;
            void* dir;
        } closedir;
    } u;
} oe_hostfs_args_t;

#endif /* _OE_HOSTFS_H */
