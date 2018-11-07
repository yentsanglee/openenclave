// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_CPIO_H
#define _FS_CPIO_H

#include "common.h"

#define FS_CPIO_FLAG_READ 0
#define FS_CPIO_FLAG_CREATE 1
#define FS_CPIO_FLAG_APPEND 2

typedef struct _fs_cpio fs_cpio_t;

typedef struct _fs_cpio_entry
{
    size_t size;
    uint32_t mode;
    char name[FS_PATH_MAX];
} fs_cpio_entry_t;

fs_cpio_t* fs_cpio_open(const char* path, uint32_t flags);

int fs_cpio_close(fs_cpio_t* cpio);

int fs_cpio_next(fs_cpio_t* cpio, fs_cpio_entry_t* entry_out);

ssize_t fs_cpio_read(fs_cpio_t* cpio, void* data, size_t size);

int fs_cpio_write_entry(fs_cpio_t* cpio, const fs_cpio_entry_t* entry);

ssize_t fs_cpio_write_data(fs_cpio_t* cpio, const void* data, size_t size);

int fs_cpio_extract(const char* source, const char* target);

#endif /* _FS_CPIO_H */
