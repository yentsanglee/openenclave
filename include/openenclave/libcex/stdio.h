/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _LIBCEX_STDIO_H
#define _LIBCEX_STDIO_H

#include <../libc/stdio.h>
#include <stdarg.h>
#include "__libcex.h"

#ifdef OE_USE_OPTEE
#define OE_FILE_SECURE_BEST_EFFORT OE_FILE_SECURE_HARDWARE
#else
#define OE_FILE_SECURE_BEST_EFFORT OE_FILE_SECURE_ENCRYPTION
#endif

LIBCEX_EXTERN_C_BEGIN

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
    OE_FILE_SECURE_HARDWARE = 1, /** Inaccessible from normal world. */
    OE_FILE_SECURE_ENCRYPTION = 2,
} oe_file_security_t;

typedef struct oe_file OE_FILE;

typedef struct oe_dir OE_DIR;

LIBCEX_INLINE int oe_feof(OE_FILE* stream)
{
    extern int libcex_oe_feof(OE_FILE * stream);
    return libcex_oe_feof(stream);
}

LIBCEX_INLINE int oe_ferror(OE_FILE* stream)
{
    extern int libcex_oe_ferror(OE_FILE * stream);
    return libcex_oe_ferror(stream);
}

LIBCEX_INLINE int oe_fflush(OE_FILE* stream)
{
    extern int libcex_oe_fflush(OE_FILE * stream);
    return libcex_oe_fflush(stream);
}

LIBCEX_INLINE char* oe_fgets(char* s, int size, OE_FILE* stream)
{
    extern char* libcex_oe_fgets(char* s, int size, OE_FILE* stream);
    return libcex_oe_fgets(s, size, stream);
}

LIBCEX_INLINE int oe_fputs(const char* s, OE_FILE* stream)
{
    extern int libcex_oe_fputs(const char* s, OE_FILE* stream);
    return libcex_oe_fputs(s, stream);
}

LIBCEX_INLINE size_t
oe_fread(void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    extern size_t libcex_oe_fread(
        void* ptr, size_t size, size_t nmemb, OE_FILE* stream);
    return libcex_oe_fread(ptr, size, nmemb, stream);
}

LIBCEX_INLINE int oe_fseek(OE_FILE* stream, long offset, int whence)
{
    extern int libcex_oe_fseek(OE_FILE * stream, long offset, int whence);
    return libcex_oe_fseek(stream, offset, whence);
}

LIBCEX_INLINE long oe_ftell(OE_FILE* stream)
{
    extern long libcex_oe_ftell(OE_FILE * stream);
    return libcex_oe_ftell(stream);
}

LIBCEX_INLINE OE_FILE* oe_fopen(
    oe_file_security_t security,
    const char* path,
    const char* mode)
{
    extern OE_FILE* libcex_oe_fopen(
        oe_file_security_t security, const char* path, const char* mode);
    return libcex_oe_fopen(security, path, mode);
}

LIBCEX_INLINE OE_FILE* oe_fopen_OE_FILE_INSECURE(
    const char* path,
    const char* mode)
{
    return oe_fopen(OE_FILE_INSECURE, path, mode);
}

LIBCEX_INLINE OE_FILE* oe_fopen_OE_FILE_SECURE_HARDWARE(
    const char* path,
    const char* mode)
{
    return oe_fopen(OE_FILE_SECURE_HARDWARE, path, mode);
}

LIBCEX_INLINE OE_FILE* oe_fopen_OE_FILE_SECURE_ENCRYPTION(
    const char* path,
    const char* mode)
{
    return oe_fopen(OE_FILE_SECURE_ENCRYPTION, path, mode);
}

LIBCEX_INLINE size_t
oe_fwrite(const void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    size_t libcex_oe_fwrite(
        const void* ptr, size_t size, size_t nmemb, OE_FILE* stream);
    return libcex_oe_fwrite(ptr, size, nmemb, stream);
}

LIBCEX_INLINE int oe_remove(oe_file_security_t security, const char* pathname)
{
    extern int libcex_oe_remove(
        oe_file_security_t security, const char* pathname);

    return libcex_oe_remove(security, pathname);
}

LIBCEX_INLINE int oe_remove_OE_FILE_INSECURE(const char* pathname)
{
    return oe_remove(OE_FILE_INSECURE, pathname);
}

LIBCEX_INLINE int oe_remove_OE_FILE_SECURE_HARDWARE(const char* pathname)
{
    return oe_remove(OE_FILE_SECURE_HARDWARE, pathname);
}

LIBCEX_INLINE int oe_remove_OE_FILE_SECURE_ENCRYPTION(const char* pathname)
{
    return oe_remove(OE_FILE_SECURE_ENCRYPTION, pathname);
}

LIBCEX_INLINE OE_DIR* oe_opendir(oe_file_security_t security, const char* name)
{
    extern OE_DIR* libcex_oe_opendir(
        oe_file_security_t security, const char* name);
    return libcex_oe_opendir(security, name);
}

LIBCEX_INLINE OE_DIR* oe_opendir_FILE_INSECURE(const char* name)
{
    return oe_opendir(OE_FILE_INSECURE, name);
}

LIBCEX_INLINE OE_DIR* oe_opendir_FILE_SECURE_HARDWARE(const char* name)
{
    return oe_opendir(OE_FILE_SECURE_HARDWARE, name);
}

LIBCEX_INLINE OE_DIR* oe_opendir_FILE_SECURE_ENCRYPTION(const char* name)
{
    return oe_opendir(OE_FILE_SECURE_ENCRYPTION, name);
}

LIBCEX_INLINE int oe_fclose(OE_FILE* stream)
{
    extern int libcex_oe_fclose(OE_FILE * stream);
    return libcex_oe_fclose(stream);
}

LIBCEX_INLINE int oe_closedir(OE_DIR* dirp)
{
    extern int libcex_oe_closedir(OE_DIR * dirp);
    return libcex_oe_closedir(dirp);
}

LIBCEX_INLINE struct oe_dirent* oe_readdir(OE_DIR* dirp)
{
    extern struct oe_dirent* libcex_oe_readdir(OE_DIR * dirp);
    return libcex_oe_readdir(dirp);
}

/*
**==============================================================================
**
** Standard POSIX definitions:
**
**==============================================================================
*/

LIBCEX_INLINE OE_FILE* __inline_oe_fopen(const char* path, const char* mode)
{
#ifdef OE_SECURE_POSIX_FILE_API
    return oe_fopen(OE_FILE_SECURE_BEST_EFFORT, path, mode);
#else
    return oe_fopen(OE_FILE_INSECURE, path, mode);
#endif
}

LIBCEX_INLINE int __inline_oe_remove(const char* pathname)
{
#ifdef OE_SECURE_POSIX_FILE_API
    return oe_remove(OE_FILE_SECURE_BEST_EFFORT, pathname);
#else
    return oe_remove(OE_FILE_SECURE_BEST_EFFORT, pathname);
#endif
}

#ifndef OE_NO_POSIX_FILE_API
/* Map POSIX API names to the OE equivalents. */
#define fopen __inline_oe_fopen
#define remove __inline_oe_remove
#define fclose oe_fclose
#define feof oe_feof
#define ferror oe_ferror
#define fflush oe_fflush
#define fread oe_fread
#define fseek oe_fseek
#define ftell oe_ftell
#define fwrite oe_fwrite
#define fputs oe_fputs
#define fgets oe_fgets
#define FILE OE_FILE
#endif /* OE_NO_POSIX_FILE_API */

#if !defined(OE_USE_OPTEE) && !defined(SEEK_SET)
#define SEEK_SET TEE_DATA_SEEK_SET
#endif

#if !defined(OE_USE_OPTEE) && !defined(SEEK_END)
#define SEEK_END TEE_DATA_SEEK_END
#endif

/*
**==============================================================================
**
** Error definitions:
**
**==============================================================================
*/

#define ENOENT 2
#define ENOMEM 12
#define EACCES 13
#define EEXIST 17
#define EINVAL 22
#define ERANGE 34

LIBCEX_EXTERN_C_END

#endif /* _LIBCEX_STDIO_H */
