// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_BITS_FS_H
#define _OE_BITS_FS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

#define OE_STDIN_FILENO 0
#define OE_STDOUT_FILENO 1
#define OE_STDERR_FILENO 2

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
#define OE_O_RDONLY        000000000
#define OE_O_WRONLY        000000001
#define OE_O_RDWR          000000002
#define OE_O_CREAT         000000100
#define OE_O_EXCL          000000200
#define OE_O_NOCTTY        000000400
#define OE_O_TRUNC         000001000
#define OE_O_APPEND        000002000
#define OE_O_NONBLOCK      000004000
#define OE_O_DSYNC         000010000
#define OE_O_SYNC          004010000
#define OE_O_RSYNC         004010000
#define OE_O_DIRECTORY     000200000
#define OE_O_NOFOLLOW      000400000
#define OE_O_CLOEXEC       002000000
#define OE_O_ASYNC         000020000
#define OE_O_DIRECT        000040000
#define OE_O_LARGEFILE     000000000
#define OE_O_NOATIME       001000000
#define OE_O_PATH          010000000
#define OE_O_TMPFILE       020200000
#define OE_O_NDELAY        O_NONBLOCK
// clang-format on

#define OE_SEEK_SET 0
#define OE_SEEK_CUR 1
#define OE_SEEK_END 2

/* Mount flags. */
#define OE_MOUNT_RDONLY 1

struct oe_dirent
{
    uint64_t d_ino;
    off_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];
};

typedef struct _oe_timespec
{
    time_t tv_sec;
    suseconds_t tv_nsec;
} oe_timespec_t;

struct oe_stat
{
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
};

#ifndef st_atime
#define st_atime st_atim.tv_sec
#endif

#ifndef st_ctime
#define st_mtime st_mtim.tv_sec
#endif

#ifndef st_ctime
#define st_ctime st_ctim.tv_sec
#endif

typedef struct _oe_iovec
{
    void* iov_base;
    size_t iov_len;
} oe_iovec_t;

int oe_mount(int devid, const char* source, const char* target, uint32_t flags);

int oe_unmount(int devid, const char* target);

int oe_open(int devid, const char* pathname, int flags, mode_t mode);

off_t oe_lseek(int fd, off_t offset, int whence);

OE_DIR* oe_opendir(int devid, const char* pathname);

struct oe_dirent* oe_readdir(OE_DIR* dir);

int oe_closedir(OE_DIR* dir);

int oe_unlink(int devid, const char* pathname);

int oe_link(int devid, const char* oldpath, const char* newpath);

int oe_rename(int devid, const char* oldpath, const char* newpath);

int oe_mkdir(int devid, const char* pathname, mode_t mode);

int oe_rmdir(int devid, const char* pathname);

int oe_stat(int devid, const char* pathname, struct oe_stat* buf);

int oe_truncate(int devid, const char* path, off_t length);

ssize_t oe_readv(int fd, const oe_iovec_t* iov, int iovcnt);

ssize_t oe_writev(int fd, const oe_iovec_t* iov, int iovcnt);

int oe_access(int devid, const char* pathname, int mode);

struct oe_dirent* oe_readdir(OE_DIR* dirp);

int oe_closedir(OE_DIR* dirp);

OE_EXTERNC_END

#endif // _OE_BITS_FS_H
