// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _oe_oefs_h
#define _oe_oefs_h

#include <openenclave/internal/defs.h>
#include <openenclave/internal/types.h>
#include "blockdev.h"
#include "fs.h"

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
