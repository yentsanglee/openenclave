// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_CPIO_H
#define _FS_CPIO_H

#include "common.h"

/*
**==============================================================================
**
** To create a CPIO archive from the current directory on Linux:
**
**     $ find . | cpio --create --format='newc' > ../archive
**
** To unpack an archive on Linux:
**
**     $ cpio -i < ../archive
**
**==============================================================================
*/

#define FS_CPIO_FLAG_READ 0
#define FS_CPIO_FLAG_CREATE 1
#define FS_CPIO_FLAG_APPEND 2

#define FS_CPIO_MODE_IFMT 00170000
#define FS_CPIO_MODE_IFSOCK 0140000
#define FS_CPIO_MODE_IFLNK 0120000
#define FS_CPIO_MODE_IFREG 0100000
#define FS_CPIO_MODE_IFBLK 0060000
#define FS_CPIO_MODE_IFDIR 0040000
#define FS_CPIO_MODE_IFCHR 0020000
#define FS_CPIO_MODE_IFIFO 0010000
#define FS_CPIO_MODE_ISUID 0004000
#define FS_CPIO_MODE_ISGID 0002000
#define FS_CPIO_MODE_ISVTX 0001000

#define FS_CPIO_MODE_IRWXU 00700
#define FS_CPIO_MODE_IRUSR 00400
#define FS_CPIO_MODE_IWUSR 00200
#define FS_CPIO_MODE_IXUSR 00100

#define FS_CPIO_MODE_IRWXG 00070
#define FS_CPIO_MODE_IRGRP 00040
#define FS_CPIO_MODE_IWGRP 00020
#define FS_CPIO_MODE_IXGRP 00010

#define FS_CPIO_MODE_IRWXO 00007
#define FS_CPIO_MODE_IROTH 00004
#define FS_CPIO_MODE_IWOTH 00002
#define FS_CPIO_MODE_IXOTH 00001

typedef struct _fs_cpio fs_cpio_t;

typedef struct _fs_cpio_entry
{
    size_t size;
    uint32_t mode;
    char name[FS_PATH_MAX];
} fs_cpio_entry_t;

fs_cpio_t* fs_cpio_open(const char* path, uint32_t flags);

int fs_cpio_close(fs_cpio_t* cpio);

int fs_cpio_read_entry(fs_cpio_t* cpio, fs_cpio_entry_t* entry_out);

ssize_t fs_cpio_read_data(fs_cpio_t* cpio, void* data, size_t size);

int fs_cpio_write_entry(fs_cpio_t* cpio, const fs_cpio_entry_t* entry);

ssize_t fs_cpio_write_data(fs_cpio_t* cpio, const void* data, size_t size);

int fs_cpio_extract(const char* source, const char* target);

#endif /* _FS_CPIO_H */
