// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_MUSL_PATCHES_STDIO_H
#define _OE_MUSL_PATCHES_STDIO_H

/* Include the original MUSL stdio.h header. */
#define __NEED_uint64_t
#include "__stdio.h"

/* This may have been defined already by <openenclave/bits/fs.h> */
#ifndef __DEFINED_oe_fs_t
typedef struct _oe_fs
{
    uint64_t __impl[16];
} oe_fs_t;
#define __DEFINED_oe_fs_t
#endif

typedef struct stat oe_stat_t;
typedef struct __dirstream DIR;

FILE* oe_fopen(oe_fs_t* fs, const char* path, const char* mode, ...);

DIR* oe_opendir(oe_fs_t* fs, const char* name);

int oe_stat(oe_fs_t* fs, const char* path, struct stat* stat);

int oe_remove(oe_fs_t* fs, const char* path);

int oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path);

int oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode);

int oe_rmdir(oe_fs_t* fs, const char* path);

extern oe_fs_t oe_hostfs;
extern oe_fs_t oe_sgxfs;
extern oe_fs_t oe_shwfs;
extern oe_fs_t oe_oefs;

/* Redefine certain path-oriented functions as needed. */
#if defined(OE_DEFAULT_FS)
#define fopen(...) oe_fopen(OE_DEFAULT_FS, __VA_ARGS__)
#define opendir(...) oe_opendir(OE_DEFAULT_FS, __VA_ARGS__)
#define stat(...) oe_stat(OE_DEFAULT_FS, __VA_ARGS__)
#define remove(...) oe_remove(OE_DEFAULT_FS, __VA_ARGS__)
#define rename(...) oe_rename(OE_DEFAULT_FS, __VA_ARGS__)
#define mkdir(...) oe_mkdir(OE_DEFAULT_FS, __VA_ARGS__)
#define rmdir(...) oe_rmdir(OE_DEFAULT_FS, __VA_ARGS__)
#endif

#endif /* _OE_MUSL_PATCHES_STDIO_H */
