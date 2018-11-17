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
    int32_t (*fs_release)(oe_fs_t* fs);

    FILE* (*fs_fopen)(
        oe_fs_t* fs,
        const char* path,
        const char* mode,
        const void* args);

    DIR* (*fs_opendir)(oe_fs_t* fs, const char* name, const void* args);

    int32_t (*fs_stat)(oe_fs_t* fs, const char* path, struct stat* stat);

    int32_t (*fs_unlink)(oe_fs_t* fs, const char* path);

    int32_t (
        *fs_rename)(oe_fs_t* fs, const char* old_path, const char* new_path);

    int32_t (*fs_mkdir)(oe_fs_t* fs, const char* path, unsigned int mode);

    int32_t (*fs_rmdir)(oe_fs_t* fs, const char* path);
};

int32_t oe_release(oe_fs_t* fs);

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

void oe_clearerr(FILE* file);

DIR* oe_opendir(oe_fs_t* fs, const char* name, const void* args);

int32_t oe_readdir(DIR* dir, struct dirent* entry, struct dirent** result);

int32_t oe_closedir(DIR* dir);

int32_t oe_stat(oe_fs_t* fs, const char* path, struct stat* stat);

int32_t oe_unlink(oe_fs_t* fs, const char* path);

int32_t oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path);

int32_t oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode);

int32_t oe_rmdir(oe_fs_t* fs, const char* path);

int oe_access(oe_fs_t* fs, const char* path, int mode);

OE_EXTERNC_END

#endif /* _OE_FS_H */
