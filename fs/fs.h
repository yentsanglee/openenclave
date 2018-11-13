// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_H
#define _FS_H

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "errno.h"

/* File system creating flags. */
#define FS_FLAG_NONE 0
#define FS_FLAG_MKFS 1
#define FS_FLAG_ENCRYPTION 2
#define FS_FLAG_AUTHENTICATION 4
#define FS_FLAG_INTEGRITY 8
#define FS_FLAG_CACHING 16

/* fs_dirent_t.d_type -- the file type. */
#define FS_DT_UNKNOWN 0
#define FS_DT_FIFO 1 /* unused */
#define FS_DT_CHR 2  /* unused */
#define FS_DT_DIR 4
#define FS_DT_BLK 6 /* unused */
#define FS_DT_REG 8
#define FS_DT_LNK 10  /* unused */
#define FS_DT_SOCK 12 /* unused */
#define FS_DT_WHT 14  /* unused */

/* fs_inode_t.i_mode -- access rights. */
#define FS_S_IFSOCK 0xC000
#define FS_S_IFLNK 0xA000
#define FS_S_IFREG 0x8000
#define FS_S_IFBLK 0x6000
#define FS_S_IFDIR 0x4000
#define FS_S_IFCHR 0x2000
#define FS_S_IFIFO 0x1000
#define FS_S_ISUID 0x0800
#define FS_S_ISGID 0x0400
#define FS_S_ISVTX 0x0200
#define FS_S_IRUSR 0x0100
#define FS_S_IWUSR 0x0080
#define FS_S_IXUSR 0x0040
#define FS_S_IRGRP 0x0020
#define FS_S_IWGRP 0x0010
#define FS_S_IXGRP 0x0008
#define FS_S_IROTH 0x0004
#define FS_S_IWOTH 0x0002
#define FS_S_IXOTH 0x0001
#define FS_S_IRWXUSR (FS_S_IRUSR | FS_S_IWUSR | FS_S_IXUSR)
#define FS_S_IRWXGRP (FS_S_IRGRP | FS_S_IWGRP | FS_S_IXGRP)
#define FS_S_IRWXOTH (FS_S_IROTH | FS_S_IWOTH | FS_S_IXOTH)
#define FS_S_IRWXALL (FS_S_IRWXUSR | FS_S_IRWXGRP | FS_S_IRWXOTH)
#define FS_S_IRWUSR (FS_S_IRUSR | FS_S_IWUSR)
#define FS_S_IRWGRP (FS_S_IRGRP | FS_S_IWGRP)
#define FS_S_IRWOTH (FS_S_IROTH | FS_S_IWOTH)
#define FS_S_IRWALL (FS_S_IRWUSR | FS_S_IRWGRP | FS_S_IRWOTH)
#define FS_S_REG_DEFAULT (FS_S_IFREG | FS_S_IRWALL)
#define FS_S_DIR_DEFAULT (FS_S_IFDIR | FS_S_IRWXALL)

// clang-format off
#define FS_O_RDONLY    000000000
#define FS_O_WRONLY    000000001
#define FS_O_RDWR      000000002
#define FS_O_CREAT     000000100
#define FS_O_EXCL      000000200
#define FS_O_NOCTTY    000000400
#define FS_O_TRUNC     000001000
#define FS_O_APPEND    000002000
#define FS_O_NONBLOCK  000004000
#define FS_O_DSYNC     000010000
#define FS_O_SYNC      004010000
#define FS_O_RSYNC     004010000
#define FS_O_DIRECTORY 000200000
#define FS_O_NOFOLLOW  000400000
#define FS_O_CLOEXEC   002000000
#define FS_O_ASYNC     000020000
#define FS_O_DIRECT    000040000
#define FS_O_LARGEFILE 000000000
#define FS_O_NOATIME   001000000
#define FS_O_PATH      010000000
#define FS_O_TMPFILE   020200000
#define FS_O_NDELAY    O_NONBLOCK
// clang-format on

/* whence parameter for fs_lseek(). */
#define FS_SEEK_SET 0
#define FS_SEEK_CUR 1
#define FS_SEEK_END 2

typedef struct _fs fs_t;
typedef struct _fs_file fs_file_t;
typedef struct _fs_dir fs_dir_t;

typedef struct _fs_dirent
{
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[FS_PATH_MAX];
    uint8_t __d_reserved;
} fs_dirent_t;

typedef struct _fs_timespec
{
    long tv_sec;
    long tv_nsec;
} fs_timespec_t;

typedef struct _fs_stat
{
    uint32_t st_dev;
    uint32_t st_ino;
    uint16_t st_mode;
    uint16_t __st_padding1;
    uint32_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_blksize;
    uint32_t st_blocks;
    uint32_t __st_padding2;
    fs_timespec_t st_atim;
    fs_timespec_t st_mtim;
    fs_timespec_t st_ctim;
} fs_stat_t;

typedef struct _fs_iovec
{
    void* iov_base;
    size_t iov_len;
} fs_iovec_t;

struct _fs
{
    fs_errno_t (*fs_release)(fs_t* fs);

    fs_errno_t (*fs_add_ref)(fs_t* fs);

    fs_errno_t (
        *fs_creat)(fs_t* fs, const char* path, uint32_t mode, fs_file_t** file);

    fs_errno_t (*fs_open)(
        fs_t* fs,
        const char* pathname,
        int flags,
        uint32_t mode,
        fs_file_t** file_out);

    fs_errno_t (*fs_lseek)(
        fs_file_t* file,
        ssize_t offset,
        int whence,
        ssize_t* offset_out);

    fs_errno_t (
        *fs_read)(fs_file_t* file, void* data, size_t size, ssize_t* nread);

    fs_errno_t (*fs_write)(
        fs_file_t* file,
        const void* data,
        size_t size,
        ssize_t* nwritten);

    fs_errno_t (*fs_close)(fs_file_t* file);

    fs_errno_t (*fs_opendir)(fs_t* fs, const char* path, fs_dir_t** dir);

    fs_errno_t (*fs_readdir)(fs_dir_t* dir, fs_dirent_t** dirent);

    fs_errno_t (*fs_closedir)(fs_dir_t* dir);

    fs_errno_t (*fs_stat)(fs_t* fs, const char* path, fs_stat_t* stat);

    fs_errno_t (*fs_link)(fs_t* fs, const char* old_path, const char* new_path);

    fs_errno_t (*fs_unlink)(fs_t* fs, const char* path);

    fs_errno_t (
        *fs_rename)(fs_t* fs, const char* old_path, const char* new_path);

    fs_errno_t (*fs_truncate)(fs_t* fs, const char* path, ssize_t length);

    fs_errno_t (*fs_mkdir)(fs_t* fs, const char* path, uint32_t mode);

    fs_errno_t (*fs_rmdir)(fs_t* fs, const char* path);
};

fs_errno_t fs_release(fs_t* fs);

fs_errno_t fs_add_ref(fs_t* fs);

fs_errno_t fs_creat(const char* pathname, uint32_t mode, int* ret);

fs_errno_t fs_open(const char* pathname, int flags, uint32_t mode, int* ret);

fs_errno_t fs_lseek(int fd, ssize_t off, int whence, ssize_t* ret);

fs_errno_t fs_readv(int fd, const fs_iovec_t* iov, int iovcnt, ssize_t* ret);

fs_errno_t fs_writev(int fd, const fs_iovec_t* iov, int iovcnt, ssize_t* ret);

fs_errno_t fs_close(int fd, int* ret);

fs_errno_t fs_opendir(const char* name, DIR** dir_out);

fs_errno_t fs_readdir(DIR* dirp, struct dirent** entry);

fs_errno_t fs_closedir(DIR* dirp, int* ret);

fs_errno_t fs_stat(const char* pathname, fs_stat_t* buf, int* ret);

fs_errno_t fs_link(const char* oldpath, const char* newpath, int* ret);

fs_errno_t fs_unlink(const char* pathname, int* ret);

fs_errno_t fs_rename(const char* oldpath, const char* newpath, int* ret);

fs_errno_t fs_truncate(const char* path, ssize_t length, int* ret);

fs_errno_t fs_mkdir(const char* pathname, uint32_t mode, int* ret);

fs_errno_t fs_rmdir(const char* pathname, int* ret);

fs_errno_t fs_access(const char* pathname, int mode, int* ret);

fs_errno_t fs_getcwd(char* buf, unsigned long size, int* ret);

fs_errno_t fs_chdir(const char* path, int* ret);

int fs_mount(fs_t* fs, const char* path);

int fs_unmount(const char* path);

fs_t* fs_lookup(const char* path, char suffix[FS_PATH_MAX]);

int fs_hostfs_new(fs_t** fs_out);

int fs_ramfs_new(fs_t** fs_out, uint32_t flags, size_t nblks);

int fs_oefs_new(
    fs_t** fs_out,
    const char* source,
    uint32_t flags,
    size_t nblks,
    const uint8_t key[FS_KEY_SIZE]);

#endif /* _FS_H */
