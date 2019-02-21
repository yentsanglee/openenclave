// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/* struct stat fields. */
dev_t st_dev;
ino_t st_ino;
nlink_t st_nlink;
mode_t st_mode;
uid_t st_uid;
gid_t st_gid;
dev_t st_rdev;
off_t st_size;
blksize_t st_blksize;
blkcnt_t st_blocks;
struct
{
    time_t tv_sec;
    suseconds_t tv_nsec;
} st_atim;
struct
{
    time_t tv_sec;
    suseconds_t tv_nsec;
} st_mtim;
struct
{
    time_t tv_sec;
    suseconds_t tv_nsec;
} st_ctim;
