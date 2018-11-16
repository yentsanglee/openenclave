#ifndef _OE_FS_H
#define _OE_FS_H

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

#define OE_FS_PATH_MAX 256

typedef struct _oe_fs oe_fs_t;
typedef struct _IO_FILE FILE;
typedef struct _oe_stat oe_stat_t;
typedef struct __dirstream oe_dir_t;
typedef struct _oe_dirent oe_dirent_t;

typedef struct _oe_dirent
{
    uint8_t d_type;
    char d_name[OE_FS_PATH_MAX];
} oe_dirent_t;

typedef struct _oe_stat
{
    uint32_t st_dev;
    uint32_t st_ino;
    uint16_t st_mode;
    uint32_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_rdev;
    uint32_t st_size;
    uint32_t st_blksize;
    uint32_t st_blocks;
} oe_stat_t;

struct _oe_fs
{
    FILE* (*fs_fopen)(
        oe_fs_t* fs,
        const char* path,
        const char* mode,
        const void* args);

    oe_dir_t* (*fs_opendir)(oe_fs_t* fs, const char* name, const void* args);

    int32_t (*fs_release)(oe_fs_t* fs);

    int32_t (*fs_stat)(oe_fs_t* fs, const char* path, oe_stat_t* stat);

    int32_t (*fs_unlink)(oe_fs_t* fs, const char* path);

    int32_t (*fs_rename)(oe_fs_t* fs, const char* old_path, const char* new_path);

    int32_t (*fs_mkdir)(oe_fs_t* fs, const char* path, unsigned int mode);

    int32_t (*fs_rmdir)(oe_fs_t* fs, const char* path);
};

FILE* oe_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode,
    const void* args);

int32_t oe_fclose(FILE* file);

size_t oe_fread(void* ptr, size_t size, size_t nmemb, FILE* file);

size_t oe_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* file);

int64_t oe_ftell(FILE* file);

int32_t oe_fseek(FILE* file, int64_t offset, int whence);

int32_t oe_fflush(FILE* file);

int32_t oe_ferror(FILE* file);

int32_t oe_feof(FILE* file);

int32_t oe_clearerr(FILE* file);

oe_dir_t* oe_opendir(oe_fs_t* fs, const char* name, const void* args);

int32_t oe_readdir(oe_dir_t* dir, oe_dirent_t* entry, oe_dirent_t** result);

int32_t oe_closedir(oe_dir_t* dir);

int32_t oe_release(oe_fs_t* fs);

int32_t oe_stat(oe_fs_t* fs, const char* path, oe_stat_t* stat);

int32_t oe_unlink(oe_fs_t* fs, const char* path);

int32_t oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path);

int32_t oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode);

int32_t oe_rmdir(oe_fs_t* fs, const char* path);

OE_EXTERNC_END

#endif /* _OE_FS_H */
