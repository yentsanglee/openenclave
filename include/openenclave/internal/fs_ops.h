// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_FS_OPS_H
#define _OE_FS_OPS_H

#include <openenclave/bits/types.h>
#include <openenclave/internal/device_ops.h>
#include <openenclave/internal/errno.h>

OE_EXTERNC_BEGIN

#define OE_PATH_MAX 4096
#define OE_BUFSIZ 8192
#define OE_FLAG_NONE 0
#define OE_FLAG_MKFS 1
#define OE_FLAG_CRYPTO 2

#define OE_DT_UNKNOWN 0
#define OE_DT_FIFO 1
#define OE_DT_CHR 2
#define OE_DT_DIR 4
#define OE_DT_BLK 6
#define OE_DT_REG 8
#define OE_DT_LNK 10
#define OE_DT_SOCK 12
#define OE_DT_WHT 14

#define OE_S_IFMT 0170000
#define OE_S_IFDIR 0040000
#define OE_S_IFCHR 0020000
#define OE_S_IFBLK 0060000
#define OE_S_IFREG 0100000
#define OE_S_IFIFO 0010000
#define OE_S_IFLNK 0120000
#define OE_S_IFSOCK 0140000

#define OE_S_ISDIR(mode) (((mode)&OE_S_IFMT) == OE_S_IFDIR)
#define OE_S_ISCHR(mode) (((mode)&OE_S_IFMT) == OE_S_IFCHR)
#define OE_S_ISBLK(mode) (((mode)&OE_S_IFMT) == OE_S_IFBLK)
#define OE_S_ISREG(mode) (((mode)&OE_S_IFMT) == OE_S_IFREG)
#define OE_S_ISFIFO(mode) (((mode)&OE_S_IFMT) == OE_S_IFIFO)
#define OE_S_ISLNK(mode) (((mode)&OE_S_IFMT) == OE_S_IFLNK)
#define OE_S_ISSOCK(mode) (((mode)&OE_S_IFMT) == OE_S_IFSOCK)

#define OE_S_ISUID 0x0800
#define OE_S_ISGID 0x0400
#define OE_S_ISVTX 0x0200
#define OE_S_IRUSR 0x0100
#define OE_S_IWUSR 0x0080
#define OE_S_IXUSR 0x0040
#define OE_S_IRGRP 0x0020
#define OE_S_IWGRP 0x0010
#define OE_S_IXGRP 0x0008
#define OE_S_IROTH 0x0004
#define OE_S_IWOTH 0x0002
#define OE_S_IXOTH 0x0001
#define OE_S_IRWXUSR (OE_S_IRUSR | OE_S_IWUSR | OE_S_IXUSR)
#define OE_S_IRWXGRP (OE_S_IRGRP | OE_S_IWGRP | OE_S_IXGRP)
#define OE_S_IRWXOTH (OE_S_IROTH | OE_S_IWOTH | OE_S_IXOTH)
#define OE_S_IRWUSR (OE_S_IRUSR | OE_S_IWUSR)
#define OE_S_IRWGRP (OE_S_IRGRP | OE_S_IWGRP)
#define OE_S_IRWOTH (OE_S_IROTH | OE_S_IWOTH)

// clang-format off
#define OE_O_RDONLY    000000000
#define OE_O_WRONLY    000000001
#define OE_O_RDWR      000000002
#define OE_O_CREAT     000000100
#define OE_O_EXCL      000000200
#define OE_O_NOCTTY    000000400
#define OE_O_TRUNC     000001000
#define OE_O_APPEND    000002000
#define OE_O_NONBLOCK  000004000
#define OE_O_DSYNC     000010000
#define OE_O_SYNC      004010000
#define OE_O_RSYNC     004010000
#define OE_O_DIRECTORY 000200000
#define OE_O_NOFOLLOW  000400000
#define OE_O_CLOEXEC   002000000
#define OE_O_ASYNC     000020000
#define OE_O_DIRECT    000040000
#define OE_O_LARGEFILE 000000000
#define OE_O_NOATIME   001000000
#define OE_O_PATH      010000000
#define OE_O_TMPFILE   020200000
#define OE_O_NDELAY    O_NONBLOCK
// clang-format on

#define OE_SEEK_SET 0
#define OE_SEEK_CUR 1
#define OE_SEEK_END 2

typedef uint32_t oe_mode_t;
typedef int64_t oe_off_t;
typedef uint64_t oe_ino_t;
typedef struct _oe_file oe_file_t;
typedef struct _oe_fs_ops oe_fs_ops_t;
typedef struct _oe_device oe_device_t;
typedef struct oe_device_t* oe_device_ptr_t;
typedef uint32_t oe_uid_t;
typedef uint32_t oe_gid_t;
typedef uint64_t oe_dev_t;
typedef uint64_t oe_nlink_t;
typedef int64_t oe_blksize_t;
typedef int64_t oe_blkcnt_t;
typedef int64_t oe_time_t;
typedef int64_t oe_suseconds_t;

struct oe_dirent
{
    uint64_t d_ino;
    oe_off_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[OE_PATH_MAX];
    uint8_t __d_reserved;
};

typedef struct _oe_timespec
{
    oe_time_t tv_sec;
    oe_suseconds_t tv_nsec;
} oe_timespec_t;

struct oe_stat
{
    oe_dev_t st_dev;
    oe_ino_t st_ino;
    oe_nlink_t st_nlink;
    oe_mode_t st_mode;
    oe_uid_t st_uid;
    oe_gid_t st_gid;
    oe_dev_t st_rdev;
    oe_off_t st_size;
    oe_blksize_t st_blksize;
    oe_blkcnt_t st_blocks;
    oe_timespec_t st_atim;
    oe_timespec_t st_mtim;
    oe_timespec_t st_ctim;
};

struct _oe_fs_ops
{
    oe_device_ops_t base;

    int (*mount)(oe_device_t* fs, const char* target, uint32_t flags);

    oe_device_t* (*open)(
        oe_device_t* fs,
        const char* pathname,
        int flags,
        oe_mode_t mode);

    oe_off_t (*lseek)(oe_device_t* file, oe_off_t offset, int whence);

    oe_device_t* (*opendir)(oe_device_t* fs, const char* path);

    struct oe_dirent* (*readdir)(oe_device_t* dir);

    int (*closedir)(oe_device_t* dir);

    int (*stat)(oe_device_t* fs, const char* pathname, struct oe_stat* buf);

    int (*link)(oe_device_t* fs, const char* oldpath, const char* newpath);

    int (*unlink)(oe_device_t* fs, const char* pathname);

    int (*rename)(oe_device_t* fs, const char* oldpath, const char* newpath);

    int (*truncate)(oe_device_t* fs, const char* path, oe_off_t length);

    int (*mkdir)(oe_device_t* fs, const char* pathname, oe_mode_t mode);

    int (*rmdir)(oe_device_t* fs, const char* pathname);
};

OE_EXTERNC_END

#endif // _OE_FS_OPS_H
