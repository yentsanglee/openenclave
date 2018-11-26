// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_H
#define _OEFS_H

#include "blkdev.h"
#include "common.h"

OE_EXTERNC_BEGIN

/* File system creating flags. */
#define OEFS_FLAG_NONE 0
#define OEFS_FLAG_MKFS 1
#define OEFS_FLAG_ENCRYPTION 2
#define OEFS_FLAG_AUTHENTICATION 4
#define OEFS_FLAG_INTEGRITY 8
#define OEFS_FLAG_CACHING 16

/* oefs_dirent_t.d_type -- the file type. */
#define OEFS_DT_UNKNOWN 0
#define OEFS_DT_FIFO 1 /* unused */
#define OEFS_DT_CHR 2  /* unused */
#define OEFS_DT_DIR 4
#define OEFS_DT_BLK 6 /* unused */
#define OEFS_DT_REG 8
#define OEFS_DT_LNK 10  /* unused */
#define OEFS_DT_SOCK 12 /* unused */
#define OEFS_DT_WHT 14  /* unused */

/* oefs_inode_t.i_mode -- access rights. */
#define OEFS_S_IFSOCK 0xC000
#define OEFS_S_IFLNK 0xA000
#define OEFS_S_IFREG 0x8000
#define OEFS_S_IFBLK 0x6000
#define OEFS_S_IFDIR 0x4000
#define OEFS_S_IFCHR 0x2000
#define OEFS_S_IFIFO 0x1000
#define OEFS_S_ISUID 0x0800
#define OEFS_S_ISGID 0x0400
#define OEFS_S_ISVTX 0x0200
#define OEFS_S_IRUSR 0x0100
#define OEFS_S_IWUSR 0x0080
#define OEFS_S_IXUSR 0x0040
#define OEFS_S_IRGRP 0x0020
#define OEFS_S_IWGRP 0x0010
#define OEFS_S_IXGRP 0x0008
#define OEFS_S_IROTH 0x0004
#define OEFS_S_IWOTH 0x0002
#define OEFS_S_IXOTH 0x0001
#define OEFS_S_IRWXUSR (OEFS_S_IRUSR | OEFS_S_IWUSR | OEFS_S_IXUSR)
#define OEFS_S_IRWXGRP (OEFS_S_IRGRP | OEFS_S_IWGRP | OEFS_S_IXGRP)
#define OEFS_S_IRWXOTH (OEFS_S_IROTH | OEFS_S_IWOTH | OEFS_S_IXOTH)
#define OEFS_S_IRWXALL (OEFS_S_IRWXUSR | OEFS_S_IRWXGRP | OEFS_S_IRWXOTH)
#define OEFS_S_IRWUSR (OEFS_S_IRUSR | OEFS_S_IWUSR)
#define OEFS_S_IRWGRP (OEFS_S_IRGRP | OEFS_S_IWGRP)
#define OEFS_S_IRWOTH (OEFS_S_IROTH | OEFS_S_IWOTH)
#define OEFS_S_IRWALL (OEFS_S_IRWUSR | OEFS_S_IRWGRP | OEFS_S_IRWOTH)
#define OEFS_S_REG_DEFAULT (OEFS_S_IFREG | OEFS_S_IRWALL)
#define OEFS_S_DIR_DEFAULT (OEFS_S_IFDIR | OEFS_S_IRWXALL)

// clang-format off
#define OEFS_O_RDONLY    000000000
#define OEFS_O_WRONLY    000000001
#define OEFS_O_RDWR      000000002
#define OEFS_O_CREAT     000000100
#define OEFS_O_EXCL      000000200
#define OEFS_O_NOCTTY    000000400
#define OEFS_O_TRUNC     000001000
#define OEFS_O_APPEND    000002000
#define OEFS_O_NONBLOCK  000004000
#define OEFS_O_DSYNC     000010000
#define OEFS_O_SYNC      004010000
#define OEFS_O_RSYNC     004010000
#define OEFS_O_DIRECTORY 000200000
#define OEFS_O_NOFOLLOW  000400000
#define OEFS_O_CLOEXEC   002000000
#define OEFS_O_ASYNC     000020000
#define OEFS_O_DIRECT    000040000
#define OEFS_O_LARGEFILE 000000000
#define OEFS_O_NOATIME   001000000
#define OEFS_O_PATH      010000000
#define OEFS_O_TMPFILE   020200000
#define OEFS_O_NDELAY    O_NONBLOCK
// clang-format on

/* whence parameter for oefs_lseek(). */
#define OEFS_SEEK_SET 0
#define OEFS_SEEK_CUR 1
#define OEFS_SEEK_END 2

typedef struct _oefs oefs_t;
typedef struct _oefs_file oefs_file_t;
typedef struct _oefs_dir oefs_dir_t;

typedef struct _oefs_dirent
{
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[OEFS_PATH_MAX];
    uint8_t __d_reserved;
} oefs_dirent_t;

typedef struct _oefs_timespec
{
    long tv_sec;
    long tv_nsec;
} oefs_timespec_t;

typedef struct _oefs_stat
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
    oefs_timespec_t st_atim;
    oefs_timespec_t st_mtim;
    oefs_timespec_t st_ctim;
} oefs_stat_t;

/* Create a new OEFS instance. */
int oefs_new(
    oefs_t** oefs_out,
    const char* source,
    uint32_t flags,
    size_t nblks,
    const uint8_t key[OEFS_KEY_SIZE]);

/* Compute required size of a file system with the given block count. */
int oefs_size(size_t nblks, size_t* size);

/* Build an OE file system on the given device. */
int oefs_mkfs(oefs_blkdev_t* dev, size_t nblks);

int oefs_release(oefs_t* oefs);

int oefs_creat(
    oefs_t* fs,
    const char* path,
    uint32_t mode,
    oefs_file_t** file_out);

int oefs_open(
    oefs_t* fs,
    const char* path,
    int flags,
    uint32_t mode,
    oefs_file_t** file_out);

int oefs_truncate(oefs_t* fs, const char* path, ssize_t length);

int oefs_lseek(
    oefs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out);

int oefs_read(
    oefs_file_t* file,
    void* data,
    size_t size,
    ssize_t* nread);

int oefs_write(
    oefs_file_t* file,
    const void* data,
    size_t size,
    ssize_t* nwritten);

int oefs_close(oefs_file_t* file);

int oefs_opendir(oefs_t* fs, const char* path, oefs_dir_t** dir);

int oefs_readdir(oefs_dir_t* dir, oefs_dirent_t** ent);

int oefs_closedir(oefs_dir_t* dir);

int oefs_stat(oefs_t* fs, const char* path, oefs_stat_t* stat);

int oefs_link(oefs_t* fs, const char* old_path, const char* new_path);

int oefs_unlink(oefs_t* fs, const char* path);

int oefs_rename(oefs_t* fs, const char* old_path, const char* new_path);

int oefs_mkdir(oefs_t* fs, const char* path, uint32_t mode);

int oefs_rmdir(oefs_t* fs, const char* path);

OE_EXTERNC_END

#endif /* _OEFS_H */
