#ifndef _OE_INTERNAL_FS_H
#define _OE_INTERNAL_FS_H

#include <dirent.h>
#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/bits/fs.h>
#include <stdio.h>

OE_EXTERNC_BEGIN

typedef struct _IO_FILE FILE;
typedef struct __dirstream DIR;
typedef struct _oe_fs_ft oe_fs_ft_t;

#define OE_FS_MAGIC 0x0ef5bd0b777e4ce1

#define OE_FILE_MAGIC 0x0EF55FE0

#define OE_FS_PATH_MAX 256

struct _IO_FILE
{
    /* Contains OE_FILE_MAGIC. */
    uint32_t magic;

    /* Padding to prevent overlap with MUSL _IO_FILE struct. */
    uint8_t padding[252];

    int (*f_fclose)(FILE* file);

    size_t (*f_fread)(void* ptr, size_t size, size_t nmemb, FILE* file);

    size_t (*f_fwrite)(const void* ptr, size_t size, size_t nmemb, FILE* file);

    int64_t (*f_ftell)(FILE* file);

    int (*f_fseek)(FILE* file, int64_t offset, int whence);

    int (*f_fflush)(FILE* file);

    int (*f_ferror)(FILE* file);

    int (*f_feof)(FILE* file);

    void (*f_clearerr)(FILE* file);
};

struct __dirstream
{
    struct dirent* (*d_readdir)(DIR* dir);

    int (*d_closedir)(DIR* dir);
};

struct _oe_fs_ft
{
    int (*fs_release)(oe_fs_t* fs);

    FILE* (
        *fs_fopen)(oe_fs_t* fs, const char* path, const char* mode, va_list ap);

    DIR* (*fs_opendir)(oe_fs_t* fs, const char* name);

    int (*fs_stat)(oe_fs_t* fs, const char* path, struct stat* stat);

    int (*fs_remove)(oe_fs_t* fs, const char* path);

    int (*fs_rename)(oe_fs_t* fs, const char* old_path, const char* new_path);

    int (*fs_mkdir)(oe_fs_t* fs, const char* path, unsigned int mode);

    int (*fs_rmdir)(oe_fs_t* fs, const char* path);
};

typedef struct _oe_fs_base
{
    uint64_t magic;
    const oe_fs_ft_t* ft;
} oe_fs_base_t;

OE_INLINE bool oe_fs_is_valid(const oe_fs_t* fs)
{
    oe_fs_base_t* base = (oe_fs_base_t*)fs;

    return base && base->magic == OE_FS_MAGIC && base->ft;
}

OE_INLINE uint64_t oe_fs_magic(const oe_fs_t* fs)
{
    return ((oe_fs_base_t*)fs)->magic;
}

OE_INLINE const oe_fs_ft_t* oe_fs_ft(const oe_fs_t* fs)
{
    return ((oe_fs_base_t*)fs)->ft;
}

OE_EXTERNC_END

#endif /* _OE_INTERNAL_FS_H */
