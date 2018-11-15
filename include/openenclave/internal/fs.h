#ifndef _OE_FS_H
#define _OE_FS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

#define OE_FS_PATH_MAX 256

typedef struct _oe_fs oe_fs_t;
typedef struct _oe_file oe_file_t;
typedef struct _oe_stat oe_stat_t;
typedef struct _oe_dir oe_dir_t;
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

struct _oe_file
{
    int32_t (*f_fclose)(oe_file_t* file);

    size_t (*f_fread)(void* ptr, size_t size, size_t nmemb, oe_file_t* file);

    size_t (
        *f_fwrite)(const void* ptr, size_t size, size_t nmemb, oe_file_t* file);

    int64_t (*f_ftell)(oe_file_t* file);

    int32_t (*f_fseek)(oe_file_t* file, int64_t offset, int whence);

    int32_t (*f_fflush)(oe_file_t* file);

    int32_t (*f_ferror)(oe_file_t* file);

    int32_t (*f_feof)(oe_file_t* file);

    int32_t (*f_clearerr)(oe_file_t* file);
};

struct _oe_dir
{
    int32_t (*d_readdir)(oe_dir_t* dir, oe_dirent_t* entry, oe_dirent_t** result);

    int32_t (*d_closedir)(oe_dir_t* dir);
};

struct _oe_fs
{
    oe_file_t* (*fs_fopen)(
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

oe_file_t* oe_fopen(
    oe_fs_t* fs,
    const char* path,
    const char* mode,
    const void* args);

int32_t oe_fclose(oe_file_t* file);

size_t oe_fread(void* ptr, size_t size, size_t nmemb, oe_file_t* file);

size_t oe_fwrite(const void* ptr, size_t size, size_t nmemb, oe_file_t* file);

int64_t oe_ftell(oe_file_t* file);

int32_t oe_fseek(oe_file_t* file, int64_t offset, int whence);

int32_t oe_fflush(oe_file_t* file);

int32_t oe_ferror(oe_file_t* file);

int32_t oe_feof(oe_file_t* file);

int32_t oe_clearerr(oe_file_t* file);

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
