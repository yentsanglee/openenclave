// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_CPIO_H
#define _FS_CPIO_H

#include "common.h"

typedef struct _fs_cpio fs_cpio_t;

typedef struct _fs_cpio_entry
{
    size_t filesize;
    uint32_t mode;
    size_t namesize;
    char name[FS_PATH_MAX];
} fs_cpio_entry_t;

fs_cpio_t* fs_cpio_open(const char* path);

int fs_cpio_close(fs_cpio_t* cpio);

int fs_cpio_next(fs_cpio_t* cpio, fs_cpio_entry_t* entry_out);

#endif /* _FS_CPIO_H */
