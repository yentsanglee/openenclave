// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_HOSTFS_H
#define _FS_HOSTFS_H

#include "fs.h"

/* 20efd84e-af47-43e4-b7eb-f1cf0d057009 */
#define FS_HOSTFS_GUID                                     \
    {                                                      \
        0x20efd84e, 0xaf47, 43e4,                          \
        {                                                  \
            0xb7, 0xeb, 0xf1, 0xcf, 0x0d, 0x05, 0x70, 0x09 \
        }                                                  \
    }

fs_errno_t hostfs_initialize(fs_t** fs);

typedef enum _fs_hostfs_op {
    FS_HOSTFS_OPEN,
    FS_HOSTFS_LSEEK,
    FS_HOSTFS_READ,
    FS_HOSTFS_WRITE,
    FS_HOSTFS_CLOSE,
    FS_HOSTFS_OPENDIR,
    FS_HOSTFS_READDIR,
    FS_HOSTFS_CLOSEDIR,
    FS_HOSTFS_STAT,
    FS_HOSTFS_LINK,
    FS_HOSTFS_UNLINK,
    FS_HOSTFS_RENAME,
    FS_HOSTFS_TRUNCATE,
    FS_HOSTFS_MKDIR,
    FS_HOSTFS_RMDIR,
} fs_hostfs_op_t;

typedef struct _fs_hostfs_ocall_args
{
    fs_hostfs_op_t op;
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
            char pathname[FS_PATH_MAX];
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
            char oldpath[FS_PATH_MAX];
            char newpath[FS_PATH_MAX];
        } link;
        struct
        {
            int ret;
            char oldpath[FS_PATH_MAX];
            char newpath[FS_PATH_MAX];
        } rename;
        struct
        {
            int ret;
            char path[FS_PATH_MAX];
        } unlink;
        struct
        {
            int ret;
            char path[FS_PATH_MAX];
            ssize_t length;
        } truncate;
        struct
        {
            int ret;
            char pathname[FS_PATH_MAX];
            uint32_t mode;
        } mkdir;
        struct
        {
            int ret;
            char pathname[FS_PATH_MAX];
        } rmdir;
    } u;
} fs_hostfs_ocall_args_t;

int fs_hostfs_ocall(fs_hostfs_ocall_args_t* args);

void fs_handle_hostfs_ocall(fs_hostfs_ocall_args_t* args);

#endif /* _FS_HOSTFS_H */
