#ifndef _OE_FS_H
#define _OE_FS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/* Use the same struct name as MUSL */
typedef struct _IO_FILE OE_FILE;

/* Use the same struct name as MUSL */
typedef struct __dirstream OE_DIR;

typedef struct stat oe_stat_t;

typedef struct dirent oe_dirent_t;

typedef struct _oe_fs
{
    uint64_t __impl[16];
} oe_fs_t;

int oe_release(oe_fs_t* fs);

bool oe_fs_set_default(oe_fs_t* fs);

oe_fs_t* oe_fs_get_default(void);

OE_FILE* oe_fopen(oe_fs_t* fs, const char* path, const char* mode, ...);

OE_DIR* oe_opendir(oe_fs_t* fs, const char* name);

int oe_stat(oe_fs_t* fs, const char* path, struct stat* stat);

int oe_remove(oe_fs_t* fs, const char* path);

int oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path);

int oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode);

int oe_rmdir(oe_fs_t* fs, const char* path);

int oe_fclose(OE_FILE* file);

size_t oe_fread(void* ptr, size_t size, size_t nmemb, OE_FILE* file);

size_t oe_fwrite(const void* ptr, size_t size, size_t nmemb, OE_FILE* file);

int64_t oe_ftell(OE_FILE* file);

int oe_fseek(OE_FILE* file, int64_t offset, int whence);

int oe_fflush(OE_FILE* file);

int oe_ferror(OE_FILE* file);

int oe_feof(OE_FILE* file);

void oe_clearerr(OE_FILE* file);

struct dirent* oe_readdir(OE_DIR* dir);

int oe_closedir(OE_DIR* dir);

OE_EXTERNC_END

#endif /* _OE_FS_H */
