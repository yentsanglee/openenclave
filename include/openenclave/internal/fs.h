#ifndef _OE_FS_H
#define _OE_FS_H

#include <dirent.h>
#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <stdio.h>
#include <sys/stat.h>

OE_EXTERNC_BEGIN

#define OE_FS_PATH_MAX 256

typedef struct _oe_fs oe_fs_t;

/* Use the same struct name as MUSL */
typedef struct _IO_FILE FILE;

/* Use the same struct name as MUSL */
typedef struct __dirstream DIR;

struct _oe_fs
{
    int (*fs_release)(oe_fs_t* fs);

    FILE* (*fs_fopen)(
        oe_fs_t* fs,
        const char* path,
        const char* mode,
        const void* args);

    DIR* (*fs_opendir)(oe_fs_t* fs, const char* name, const void* args);

    int (*fs_stat)(oe_fs_t* fs, const char* path, struct stat* stat);

    int (*fs_remove)(oe_fs_t* fs, const char* path);

    int (*fs_rename)(oe_fs_t* fs, const char* old_path, const char* new_path);

    int (*fs_mkdir)(oe_fs_t* fs, const char* path, unsigned int mode);

    int (*fs_rmdir)(oe_fs_t* fs, const char* path);
};

int oe_release(oe_fs_t* fs);

void oe_fs_set_default(oe_fs_t* fs);

oe_fs_t* oe_fs_get_default(void);

FILE* oe_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode,
    const void* args);

DIR* oe_opendir(oe_fs_t* fs, const char* name, const void* args);

int oe_stat(oe_fs_t* fs, const char* path, struct stat* stat);

int oe_remove(oe_fs_t* fs, const char* path);

int oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path);

int oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode);

int oe_rmdir(oe_fs_t* fs, const char* path);

OE_EXTERNC_END

#endif /* _OE_FS_H */
