/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _STDIOV2_H
#define _STDIOV2_H

#include <stdint.h>
#include <stdarg.h>

#ifdef OE_USE_OPTEE
# define OE_FILE_SECURE_BEST_EFFORT OE_FILE_SECURE_HARDWARE
#else
# define OE_FILE_SECURE_BEST_EFFORT OE_FILE_SECURE_ENCRYPTION
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
**==============================================================================
**
** oe-prefixed definitions:
**
**==============================================================================
*/

typedef enum
{
    OE_FILE_INSECURE = 0,
    OE_FILE_SECURE_HARDWARE = 1,     /** Inaccessible from normal world. */
    OE_FILE_SECURE_ENCRYPTION = 2,
} oe_file_security_t;

typedef struct oe_file OE_FILE;

typedef struct _oe_device OE_DIR;

int oe_fclose(OE_FILE* stream);

int oe_feof(OE_FILE* stream);

int oe_ferror(OE_FILE *stream);

int oe_fflush(OE_FILE *stream);

char *oe_fgets(char *s, int size, OE_FILE *stream);

int oe_fputs(const char *s, OE_FILE *stream);

size_t oe_fread(void *ptr, size_t size, size_t nmemb, OE_FILE *stream);

int oe_fseek(OE_FILE *stream, long offset, int whence);

int64_t oe_ftell(OE_FILE *stream);

size_t oe_fwrite(const void *ptr, size_t size, size_t nmemb, OE_FILE *stream);

OE_FILE *oe_fopen(
    oe_file_security_t security,
    const char *path,
    const char *mode);

static inline OE_FILE *oe_fopen_OE_FILE_INSECURE(
    const char* path, const char* mode)
{
    return oe_fopen(OE_FILE_INSECURE, path, mode);
}

static inline OE_FILE *oe_fopen_OE_FILE_SECURE_HARDWARE(
    const char* path, const char* mode)
{
    return oe_fopen(OE_FILE_SECURE_HARDWARE, path, mode);
}

static inline OE_FILE *oe_fopen_OE_FILE_SECURE_ENCRYPTION(
    const char* path, const char* mode)
{
    return oe_fopen(OE_FILE_SECURE_ENCRYPTION, path, mode);
}

int oe_remove(oe_file_security_t security, const char *pathname);

static inline int oe_remove_OE_FILE_INSECURE(const char *pathname)
{
    return oe_remove(OE_FILE_INSECURE, pathname);
}

static inline int oe_remove_OE_FILE_SECURE_HARDWARE(const char *pathname)
{
    return oe_remove(OE_FILE_SECURE_HARDWARE, pathname);
}

static inline int oe_remove_OE_FILE_SECURE_ENCRYPTION(const char *pathname)
{
    return oe_remove(OE_FILE_SECURE_ENCRYPTION, pathname);
}

OE_DIR *oe_opendir_ex(oe_file_security_t security, const char *name);

static inline OE_DIR *oe_opendir_FILE_INSECURE(const char *name)
{
    return oe_opendir_ex(OE_FILE_INSECURE, name);
}

static inline OE_DIR *oe_opendir_FILE_SECURE_HARDWARE(const char *name)
{
    return oe_opendir_ex(OE_FILE_SECURE_HARDWARE, name);
}

static inline OE_DIR *oe_opendir_FILE_SECURE_ENCRYPTION(const char *name)
{
    return oe_opendir_ex(OE_FILE_SECURE_ENCRYPTION, name);
}

int oe_closedir(OE_DIR *dirp);

struct oe_dirent *oe_readdir(OE_DIR *dirp);

/*
**==============================================================================
**
** Standrad POSIX definitions:
**
**==============================================================================
*/

#ifndef OE_NO_POSIX_FILE_API

typedef OE_FILE FILE;

static inline OE_FILE *fopen(const char *path, const char *mode)
{
#ifdef OE_SECURE_POSIX_FILE_API
    return oe_fopen(OE_FILE_SECURE_BEST_EFFORT, path, mode);
#else
    return oe_fopen(OE_FILE_INSECURE, path, mode);
#endif
}

static inline int remove(const char *pathname)
{
#ifdef OE_SECURE_POSIX_FILE_API
    return oe_remove(OE_FILE_SECURE_BEST_EFFORT, pathname);
#else
    return oe_remove(OE_FILE_SECURE_BEST_EFFORT, pathname);
#endif
}

static inline int fclose(OE_FILE *stream)
{
    return oe_fclose(stream);
}

static inline int feof(OE_FILE *stream)
{
    return oe_feof(stream);
}

static inline int ferror(OE_FILE *stream)
{
    return oe_ferror(stream);
}

static inline int fflush(OE_FILE *stream)
{
    return oe_fflush(stream);
}

static inline size_t fread(
    void *ptr, size_t size, size_t nmemb, OE_FILE *stream)
{
    return oe_fread(ptr, size, nmemb, stream);
}

static inline size_t fwrite(
    const void *ptr, size_t size, size_t nmemb, OE_FILE *stream)
{
    return oe_fwrite(ptr, size, nmemb, stream);
}

static inline int fseek(OE_FILE *stream, long offset, int whence)
{
    return oe_fseek(stream, offset, whence);
}

static inline long ftell(OE_FILE *stream)
{
    return oe_ftell(stream);
}

static inline long fputs(const char* s, OE_FILE *stream)
{
    return oe_fputs(s, stream);
}

static inline char *fgets(char *s, int size, OE_FILE *stream)
{
    return oe_fgets(s, size, stream);
}

int fprintf(FILE *stream, const char *format, ...);

int vfprintf(FILE* stream, const char* format, va_list ap);

int vprintf(const char* format, va_list ap);

int printf(const char* format, ...);

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;

#define stdin  (stdin)
#define stdout (stdout)
#define stderr (stderr)

#if !defined(OE_USE_OPTEE) && !defined(SEEK_SET)
#define SEEK_SET TEE_DATA_SEEK_SET
#endif

#if !defined(OE_USE_OPTEE) && !defined(SEEK_END)
#define SEEK_END TEE_DATA_SEEK_END
#endif

#endif /* OE_NO_POSIX_FILE_API */

/*
**==============================================================================
**
** Error definitions:
**
**==============================================================================
*/

#define ENOENT     2
#define ENOMEM    12
#define EACCES    13
#define EEXIST    17
#define EINVAL    22
#define ERANGE    34

#ifdef __cplusplus
}
#endif

#endif /* _STDIOV2_H */
