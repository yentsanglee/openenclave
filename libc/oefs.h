// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _oe_oefs_h
#define _oe_oefs_h

#include <openenclave/internal/defs.h>
#include <openenclave/internal/types.h>
#include "blockdev.h"

#define OEFS_PATH_MAX 256
#define OEFS_BLOCK_SIZE 512
#define OEFS_BITS_PER_BLOCK (OEFS_BLOCK_SIZE * 8)
#define OEFS_SUPER_BLOCK_MAGIC 0x0EF55FE0
#define OEFS_INODE_MAGIC 0x0120DD021

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

/* whence parameter for oefs_lseek(). */
#define OEFS_SEEK_SET 0
#define OEFS_SEEK_CUR 1
#define OEFS_SEEK_END 2

typedef struct _oefs_super_block
{
    /* Magic number: OEFS_SUPER_BLOCK_MAGIC. */
    uint32_t s_magic;

    /* The total number of blocks in the file system. */
    uint32_t s_nblocks;

    /* The number of free blocks. */
    uint32_t s_free_blocks;

    /* Reserved. */
    uint8_t s_reserved[500];
} oefs_super_block_t;

OE_STATIC_ASSERT(sizeof(oefs_super_block_t) == OEFS_BLOCK_SIZE);

typedef struct _oefs_inode
{
    uint32_t i_magic;

    /* Access rights. */
    uint16_t i_mode;

    /* Owner user id */
    uint16_t i_uid;

    /* Owner group id */
    uint16_t i_gid;

    /* The number of links to this inode. */
    uint16_t i_links;

    /* Size of this file. */
    uint32_t i_size;

    /* File times. */
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;

    /* Total number of 512-byte blocks in this file. */
    uint32_t i_nblocks;

    /* The next blknos block. */
    uint32_t i_next;

    /* Reserved. */
    uint32_t i_reserved[6];

    /* Blocks comprising this file. */
    uint32_t i_blocks[112];

} oefs_inode_t;

OE_STATIC_ASSERT(sizeof(oefs_inode_t) == OEFS_BLOCK_SIZE);

typedef struct _oefs_bnode
{
    /* The next blknos block. */
    uint32_t b_next;

    /* Blocks comprising this file. */
    uint32_t b_blocks[127];

} oefs_bnode_t;

OE_STATIC_ASSERT(sizeof(oefs_bnode_t) == OEFS_BLOCK_SIZE);

typedef struct _oefs_dirent
{
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[OEFS_PATH_MAX];
    uint8_t __d_reserved;
} oefs_dirent_t;

typedef struct _oefs_dir oefs_dir_t;

OE_STATIC_ASSERT(sizeof(oefs_dirent_t) == 268);

typedef struct _oefs_stat
{
    uint32_t st_dev;
    uint32_t st_ino;
    uint16_t st_mode;
    uint16_t __st_padding;
    uint32_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_blksize;
    uint32_t st_blocks;
    uint32_t st_atime;
    uint32_t st_mtime;
    uint32_t st_ctime;
} oefs_stat_t;

OE_STATIC_ASSERT(sizeof(oefs_stat_t) == 48);

typedef struct _oefs
{
    oe_block_dev_t* dev;
    oefs_super_block_t sb;
    uint8_t* bitmap;
    size_t bitmap_size;
    bool dirty;
} oefs_t;

typedef struct _oefs_block
{
    uint8_t data[OEFS_BLOCK_SIZE];
} oefs_block_t;

typedef enum _oefs_result {
    OEFS_OK,
    OEFS_BAD_PARAMETER,
    OEFS_FAILED,
    OEFS_NOT_FOUND,
    OEFS_ALREADY_EXISTS,
    OEFS_BUFFER_OVERFLOW,
    OEFS_BOUNDS,
} oefs_result_t;

typedef struct _oefs_file oefs_file_t;

/* Compute required size of a file system with the given block count. */
oefs_result_t oefs_size(size_t nblocks, size_t* size);

/* Build an OE file system on the given device. */
oefs_result_t oefs_mkfs(oe_block_dev_t* dev, size_t nblocks);

/* Initialize the oefs instance from the given device. */
oefs_result_t oefs_initialize(oefs_t** oefs, oe_block_dev_t* dev);

/* Release the oefs instance. */
oefs_result_t oefs_release(oefs_t* oefs);

oefs_result_t oefs_read(
    oefs_file_t* file,
    void* data,
    uint32_t size,
    int32_t* nread);

oefs_result_t oefs_write(
    oefs_file_t* file,
    const void* data,
    uint32_t size,
    int32_t* nwritten);

oefs_result_t oefs_close(oefs_file_t* file);

oefs_result_t oefs_opendir(oefs_t* oefs, const char* path, oefs_dir_t** dir);

oefs_result_t oefs_readdir(oefs_dir_t* dir, oefs_dirent_t** dirent);

oefs_result_t oefs_closedir(oefs_dir_t* dir);

oefs_result_t oefs_open(
    oefs_t* oefs,
    const char* pathname,
    int flags,
    uint32_t mode,
    oefs_file_t** file_out);

oefs_result_t oefs_load(
    oefs_t* oefs,
    const char* path,
    void** data_out,
    size_t* size_out);

oefs_result_t oefs_mkdir(oefs_t* oefs, const char* path, uint32_t mode);

oefs_result_t oefs_create(
    oefs_t* oefs,
    const char* path,
    uint32_t mode,
    oefs_file_t** file);

oefs_result_t oefs_link(
    oefs_t* oefs,
    const char* old_path,
    const char* new_path);

oefs_result_t oefs_rename(
    oefs_t* oefs,
    const char* old_path,
    const char* new_path);

oefs_result_t oefs_unlink(oefs_t* oefs, const char* path);

oefs_result_t oefs_truncate(oefs_t* oefs, const char* path);

oefs_result_t oefs_rmdir(oefs_t* oefs, const char* path);

oefs_result_t oefs_stat(oefs_t* oefs, const char* path, oefs_stat_t* stat);

oefs_result_t oefs_lseek(
    oefs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out);

#endif /* _oe_oefs_h */
