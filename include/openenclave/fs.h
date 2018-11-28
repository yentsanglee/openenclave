#ifndef _OE_FS_H
#define _OE_FS_H

#include <dirent.h>
#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <stdio.h>
#include <sys/stat.h>

OE_EXTERNC_BEGIN

/* Use the same struct name as MUSL */
typedef struct _IO_FILE FILE;

/* Use the same struct name as MUSL */
typedef struct __dirstream DIR;

typedef struct _oe_fs
{
    uint64_t __impl[16];
} oe_fs_t;

int oe_release(oe_fs_t* fs);

bool oe_fs_set_default(oe_fs_t* fs);

oe_fs_t* oe_fs_get_default(void);

FILE* oe_fopen(oe_fs_t* fs, const char* path, const char* mode, ...);

DIR* oe_opendir(oe_fs_t* fs, const char* name);

int oe_stat(oe_fs_t* fs, const char* path, struct stat* stat);

int oe_remove(oe_fs_t* fs, const char* path);

int oe_rename(oe_fs_t* fs, const char* old_path, const char* new_path);

int oe_mkdir(oe_fs_t* fs, const char* path, unsigned int mode);

int oe_rmdir(oe_fs_t* fs, const char* path);

int oe_fclose(FILE* file);

size_t oe_fread(void* ptr, size_t size, size_t nmemb, FILE* file);

size_t oe_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* file);

int64_t oe_ftell(FILE* file);

int oe_fseek(FILE* file, int64_t offset, int whence);

int oe_fflush(FILE* file);

int oe_ferror(FILE* file);

int oe_feof(FILE* file);

void oe_clearerr(FILE* file);

struct dirent* oe_readdir(DIR* dir);

int oe_closedir(DIR* dir);

OE_EXTERNC_END

#endif /* _OE_FS_H */
