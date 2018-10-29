// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _oe_oefs_h
#define _oe_oefs_h

#include <openenclave/internal/defs.h>
#include <openenclave/internal/types.h>
#include "blockdev.h"
#include "fs.h"

#define OEFS_BITS_PER_BLOCK (FS_BLOCK_SIZE * 8)
#define OEFS_SUPER_BLOCK_MAGIC 0x0EF55FE0
#define OEFS_INODE_MAGIC 0x0120DD021

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

OE_STATIC_ASSERT(sizeof(oefs_super_block_t) == FS_BLOCK_SIZE);

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

OE_STATIC_ASSERT(sizeof(oefs_inode_t) == FS_BLOCK_SIZE);

typedef struct _oefs_bnode
{
    /* The next blknos block. */
    uint32_t b_next;

    /* Blocks comprising this file. */
    uint32_t b_blocks[127];

} oefs_bnode_t;

OE_STATIC_ASSERT(sizeof(oefs_bnode_t) == FS_BLOCK_SIZE);

OE_STATIC_ASSERT(sizeof(fs_dirent_t) == 268);

OE_STATIC_ASSERT(sizeof(fs_stat_t) == 48);

typedef struct _oefs
{
    fs_t base;
    /* ATTN: add magic number here. */
    oe_block_dev_t* dev;

    union
    {
        const oefs_super_block_t read;
        // This field should only be accessed with the _sb_write() method, 
        // which sets the oefs_t.dirty flag.
        oefs_super_block_t __write;
    }
    sb;

    union
    {
        // This field should only be accessed with the _bitmap_write() method,
        // which sets the oefs_t.dirty flag.
        uint8_t* __write;
        const uint8_t* read;
    }
    bitmap;

    oefs_super_block_t sb_copy;
    uint8_t* bitmap_copy;
    size_t bitmap_size;

    bool dirty;
} oefs_t;

typedef struct _oefs_block
{
    uint8_t data[FS_BLOCK_SIZE];
} oefs_block_t;

/* Compute required size of a file system with the given block count. */
oe_errno_t oefs_size(size_t nblocks, size_t* size);

/* Build an OE file system on the given device. */
oe_errno_t oefs_mkfs(oe_block_dev_t* dev, size_t nblocks);

/* Initialize the oefs instance from the given device. */
oe_errno_t oefs_initialize(fs_t** fs, oe_block_dev_t* dev);

/* Release the oefs instance. */
oe_errno_t oefs_release(fs_t* fs);

oe_errno_t oefs_read(
    fs_file_t* file,
    void* data,
    uint32_t size,
    int32_t* nread);

oe_errno_t oefs_write(
    fs_file_t* file,
    const void* data,
    uint32_t size,
    int32_t* nwritten);

oe_errno_t oefs_close(fs_file_t* file);

oe_errno_t oefs_opendir(fs_t* fs, const char* path, fs_dir_t** dir);

oe_errno_t oefs_readdir(fs_dir_t* dir, fs_dirent_t** dirent);

oe_errno_t oefs_closedir(fs_dir_t* dir);

oe_errno_t oefs_open(
    fs_t* fs,
    const char* pathname,
    int flags,
    uint32_t mode,
    fs_file_t** file_out);

oe_errno_t oefs_load(
    fs_t* fs,
    const char* path,
    void** data_out,
    size_t* size_out);

oe_errno_t oefs_mkdir(fs_t* fs, const char* path, uint32_t mode);

oe_errno_t oefs_create(
    fs_t* fs,
    const char* path,
    uint32_t mode,
    fs_file_t** file);

oe_errno_t oefs_link(
    fs_t* fs,
    const char* old_path,
    const char* new_path);

oe_errno_t oefs_rename(
    fs_t* fs,
    const char* old_path,
    const char* new_path);

oe_errno_t oefs_unlink(fs_t* fs, const char* path);

oe_errno_t oefs_truncate(fs_t* fs, const char* path);

oe_errno_t oefs_rmdir(fs_t* fs, const char* path);

oe_errno_t oefs_stat(fs_t* fs, const char* path, fs_stat_t* stat);

oe_errno_t oefs_lseek(
    fs_file_t* file,
    ssize_t offset,
    int whence,
    ssize_t* offset_out);

#endif /* _oe_oefs_h */
