/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _IOT_STDIO_H
#define _IOT_STDIO_H

#include <../libc/stdio.h>
#include <openenclave/iot/bits/common.h>

#ifdef OE_USE_OPTEE
#define OE_FILE_SECURE_BEST_EFFORT OE_FILE_SECURE_HARDWARE
#else
#define OE_FILE_SECURE_BEST_EFFORT OE_FILE_SECURE_ENCRYPTION
#endif

IOT_EXTERN_C_BEGIN

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

typedef struct _OE_FILE OE_FILE;

typedef struct _OE_DIR OE_DIR;

IOT_INLINE int oe_feof(OE_FILE* stream)
{
    extern int iot_oe_feof(OE_FILE * stream);
    return iot_oe_feof(stream);
}

IOT_INLINE int oe_ferror(OE_FILE* stream)
{
    extern int iot_oe_ferror(OE_FILE * stream);
    return iot_oe_ferror(stream);
}

IOT_INLINE int oe_fflush(OE_FILE* stream)
{
    extern int iot_oe_fflush(OE_FILE * stream);
    return iot_oe_fflush(stream);
}

IOT_INLINE char* oe_fgets(char* s, int size, OE_FILE* stream)
{
    extern char* iot_oe_fgets(char* s, int size, OE_FILE* stream);
    return iot_oe_fgets(s, size, stream);
}

IOT_INLINE int oe_fputs(const char* s, OE_FILE* stream)
{
    extern int iot_oe_fputs(const char* s, OE_FILE* stream);
    return iot_oe_fputs(s, stream);
}

IOT_INLINE size_t
oe_fread(void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    extern size_t iot_oe_fread(
        void* ptr, size_t size, size_t nmemb, OE_FILE* stream);
    return iot_oe_fread(ptr, size, nmemb, stream);
}

IOT_INLINE int oe_fseek(OE_FILE* stream, long offset, int whence)
{
    extern int iot_oe_fseek(OE_FILE * stream, long offset, int whence);
    return iot_oe_fseek(stream, offset, whence);
}

IOT_INLINE long oe_ftell(OE_FILE* stream)
{
    extern long iot_oe_ftell(OE_FILE * stream);
    return iot_oe_ftell(stream);
}

IOT_INLINE OE_FILE* oe_fopen(
    oe_file_security_t security,
    const char* path,
    const char* mode)
{
    extern OE_FILE* iot_oe_fopen(
        oe_file_security_t security, const char* path, const char* mode);
    return iot_oe_fopen(security, path, mode);
}

IOT_INLINE OE_FILE* oe_fopen_OE_FILE_INSECURE(
    const char* path,
    const char* mode)
{
    return oe_fopen(OE_FILE_INSECURE, path, mode);
}

IOT_INLINE OE_FILE* oe_fopen_OE_FILE_SECURE_HARDWARE(
    const char* path,
    const char* mode)
{
    return oe_fopen(OE_FILE_SECURE_HARDWARE, path, mode);
}

IOT_INLINE OE_FILE* oe_fopen_OE_FILE_SECURE_ENCRYPTION(
    const char* path,
    const char* mode)
{
    return oe_fopen(OE_FILE_SECURE_ENCRYPTION, path, mode);
}

IOT_INLINE OE_FILE* oe_fopen_OE_FILE_SECURE_BEST_EFFORT(
    const char* path,
    const char* mode)
{
    return oe_fopen(OE_FILE_SECURE_BEST_EFFORT, path, mode);
}

IOT_INLINE size_t
oe_fwrite(const void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    size_t iot_oe_fwrite(
        const void* ptr, size_t size, size_t nmemb, OE_FILE* stream);
    return iot_oe_fwrite(ptr, size, nmemb, stream);
}

IOT_INLINE int oe_remove(oe_file_security_t security, const char* pathname)
{
    extern int iot_oe_remove(
        oe_file_security_t security, const char* pathname);

    return iot_oe_remove(security, pathname);
}

IOT_INLINE int oe_remove_OE_FILE_INSECURE(const char* pathname)
{
    return oe_remove(OE_FILE_INSECURE, pathname);
}

IOT_INLINE int oe_remove_OE_FILE_SECURE_HARDWARE(const char* pathname)
{
    return oe_remove(OE_FILE_SECURE_HARDWARE, pathname);
}

IOT_INLINE int oe_remove_OE_FILE_SECURE_ENCRYPTION(const char* pathname)
{
    return oe_remove(OE_FILE_SECURE_ENCRYPTION, pathname);
}

IOT_INLINE int oe_remove_OE_FILE_SECURE_BEST_EFFORT(const char* pathname)
{
    return oe_remove(OE_FILE_SECURE_BEST_EFFORT, pathname);
}

IOT_INLINE OE_DIR* oe_opendir(oe_file_security_t security, const char* name)
{
    extern OE_DIR* iot_oe_opendir(
        oe_file_security_t security, const char* name);
    return iot_oe_opendir(security, name);
}

IOT_INLINE OE_DIR* oe_opendir_FILE_INSECURE(const char* name)
{
    return oe_opendir(OE_FILE_INSECURE, name);
}

IOT_INLINE OE_DIR* oe_opendir_FILE_SECURE_HARDWARE(const char* name)
{
    return oe_opendir(OE_FILE_SECURE_HARDWARE, name);
}

IOT_INLINE OE_DIR* oe_opendir_FILE_SECURE_ENCRYPTION(const char* name)
{
    return oe_opendir(OE_FILE_SECURE_ENCRYPTION, name);
}

IOT_INLINE int oe_fclose(OE_FILE* stream)
{
    extern int iot_oe_fclose(OE_FILE * stream);
    return iot_oe_fclose(stream);
}

IOT_INLINE int oe_closedir(OE_DIR* dirp)
{
    extern int iot_oe_closedir(OE_DIR * dirp);
    return iot_oe_closedir(dirp);
}

IOT_INLINE struct oe_dirent* oe_readdir(OE_DIR* dirp)
{
    extern struct oe_dirent* iot_oe_readdir(OE_DIR * dirp);
    return iot_oe_readdir(dirp);
}

/*
**==============================================================================
**
** Standard POSIX definitions:
**
**==============================================================================
*/

IOT_INLINE OE_FILE* __inline_oe_fopen(const char* path, const char* mode)
{
#ifdef OE_SECURE_POSIX_FILE_API
    return oe_fopen(OE_FILE_SECURE_BEST_EFFORT, path, mode);
#else
    return oe_fopen(OE_FILE_INSECURE, path, mode);
#endif
}

IOT_INLINE int __inline_oe_remove(const char* pathname)
{
#ifdef OE_SECURE_POSIX_FILE_API
    return oe_remove(OE_FILE_SECURE_BEST_EFFORT, pathname);
#else
    return oe_remove(OE_FILE_INSECURE, pathname);
#endif
}

/* Map POSIX API names to the OE equivalents. */
#ifndef OE_NO_POSIX_FILE_API

#ifdef OE_SECURE_POSIX_FILE_API
#define remove oe_remove_OE_FILE_SECURE_BEST_EFFORT
#define fopen oe_fopen_OE_FILE_SECURE_BEST_EFFORT
#else
#define fopen oe_fopen_OE_FILE_INSECURE
#define remove oe_remove_OE_FILE_INSECURE
#endif

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

IOT_EXTERN_C_END

#endif /* _IOT_STDIO_H */
