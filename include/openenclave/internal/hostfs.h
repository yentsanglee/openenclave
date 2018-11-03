// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_HOSTFS_H
#define _OE_HOSTFS_H

#include <limits.h>
#include <openenclave/bits/defs.h>
#include <openenclave/internal/defs.h>
#include <stdint.h>

#define HOSTFS_PATH_MAX 256

typedef enum _oe_hostfs_op {
    OE_HOSTFS_OPEN,
    OE_HOSTFS_LSEEK,
    OE_HOSTFS_READ,
    OE_HOSTFS_WRITE,
    OE_HOSTFS_CLOSE,
    OE_HOSTFS_OPENDIR,
    OE_HOSTFS_READDIR,
    OE_HOSTFS_CLOSEDIR,
    OE_HOSTFS_STAT,
    OE_HOSTFS_LINK,
    OE_HOSTFS_UNLINK,
    OE_HOSTFS_RENAME,
    OE_HOSTFS_TRUNCATE,
    OE_HOSTFS_MKDIR,
    OE_HOSTFS_RMDIR,
    __OE_HOSTFS_MAX = OE_ENUM_MAX,
} oe_hostfs_op_t;

typedef struct _oe_hostfs_args
{
    oe_hostfs_op_t op;
    int err; /* errno value */

    union {
        struct
        {
            int ret;
            const char* pathname;
            int flags;
            uint32_t mode;
        } open;
        struct
        {
            int64_t ret;
            int fd;
            int64_t offset;
            int whence;
        } lseek;
        struct
        {
            int64_t ret;
            int fd;
            void* buf;
            uint64_t count;
        } read;
        struct
        {
            int64_t ret;
            int fd;
            void* buf;
            uint64_t count;
        } write;
        struct
        {
            int ret;
            int fd;
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
                uint64_t d_ino;
                uint64_t d_off;
                uint16_t d_reclen;
                uint8_t d_type;
                char d_name[256];
            } buf;
            void* entry;
            void* dir;
        } readdir;
        struct
        {
            int ret;
            void* dir;
        } closedir;
        struct
        {
            int ret;
            char pathname[HOSTFS_PATH_MAX];
            struct
            {
                uint32_t st_dev;
                uint32_t st_ino;
                uint16_t st_mode;
                uint32_t st_nlink;
                uint16_t st_uid;
                uint16_t st_gid;
                uint32_t st_rdev;
                uint32_t st_size;
                uint32_t st_blksize;
                uint32_t st_blocks;
                struct
                {
                    uint64_t tv_sec;
                    uint64_t tv_nsec;
                } st_atim;
                struct
                {
                    uint64_t tv_sec;
                    uint64_t tv_nsec;
                } st_mtim;
                struct
                {
                    uint64_t tv_sec;
                    uint64_t tv_nsec;
                } st_ctim;
            } buf;
        } stat;
        struct
        {
            int ret;
            char oldpath[HOSTFS_PATH_MAX];
            char newpath[HOSTFS_PATH_MAX];
        } link;
        struct
        {
            int ret;
            char oldpath[HOSTFS_PATH_MAX];
            char newpath[HOSTFS_PATH_MAX];
        } rename;
        struct
        {
            int ret;
            char path[HOSTFS_PATH_MAX];
        } unlink;
        struct
        {
            int ret;
            char path[HOSTFS_PATH_MAX];
            ssize_t length;
        } truncate;
        struct
        {
            int ret;
            char pathname[HOSTFS_PATH_MAX];
            uint32_t mode;
        } mkdir;
        struct
        {
            int ret;
            char pathname[HOSTFS_PATH_MAX];
        } rmdir;
    } u;
} oe_hostfs_args_t;

#endif /* _OE_HOSTFS_H */
