/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _OE_IOT_STDIOBEGIN_H
#define _OE_IOT_STDIOBEGIN_H

#include <../libc/stdio.h>
#include <dirent.h>
#include <openenclave/bits/device.h>
#include <openenclave/iot/bits/common.h>
#include <stdarg.h>
#include <stdio.h>

IOT_EXTERN_C_BEGIN

#define _FILE_DEFINED

#if !defined(_OE_ENCLAVE_H)
#define _OE_ENCLAVE_H
#define _STDIOBEGIN_DEFINED_OE_ENCLAVE_H
#endif

typedef struct _IO_FILE FILE;
typedef struct oe_file OE_FILE;
typedef struct oe_dir OE_DIR;

IOT_INLINE int __oe_fclose(OE_FILE* stream)
{
    return fclose((FILE*)stream);
}

IOT_INLINE int __oe_feof(OE_FILE* stream)
{
    return feof((FILE*)stream);
}

IOT_INLINE int __oe_ferror(OE_FILE* stream)
{
    return ferror((FILE*)stream);
}

IOT_INLINE int __oe_fflush(OE_FILE* stream)
{
    return fflush((FILE*)stream);
}

IOT_INLINE char* __oe_fgets(char* s, int size, OE_FILE* stream)
{
    return fgets(s, size, (FILE*)stream);
}

IOT_INLINE int __oe_fprintf(
    OE_FILE* const stream,
    char const* const format,
    ...)
{
    va_list ap;
    va_start(ap, format);
    int n = vfprintf((FILE*)stream, format, ap);
    va_end(ap);
    return n;
}

IOT_INLINE int __oe_fputs(const char* s, OE_FILE* stream)
{
    return fputs(s, (FILE*)stream);
}

IOT_INLINE size_t
__oe_fread(void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    return fread(ptr, size, nmemb, (FILE*)stream);
}

IOT_INLINE int __oe_fseek(OE_FILE* stream, long offset, int whence)
{
    return fseek((FILE*)stream, offset, whence);
}

IOT_INLINE long __oe_ftell(OE_FILE* stream)
{
    return ftell((FILE*)stream);
}

IOT_INLINE int __oe_vfprintf(
    OE_FILE* stream,
    const char* format,
    va_list argptr)
{
    return vfprintf((FILE*)stream, format, argptr);
}

IOT_INLINE size_t
__oe_fwrite(const void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    return fwrite(ptr, size, nmemb, (FILE*)stream);
}

IOT_INLINE OE_FILE* __oe_fopen_OE_FILE_INSECURE(
    const char* path,
    const char* mode)
{
    return oe_device_fopen(OE_DEVICE_ID_INSECURE_FS, path, mode);
}

IOT_INLINE OE_FILE* __oe_fopen_OE_FILE_SECURE_HARDWARE(
    const char* path,
    const char* mode)
{
    return oe_device_fopen(OE_DEVICE_ID_SECURE_HARDWARE_FS, path, mode);
}

IOT_INLINE OE_FILE* __oe_fopen_OE_FILE_SECURE_ENCRYPTION(
    const char* path,
    const char* mode)
{
    return oe_device_fopen(OE_DEVICE_ID_ENCRYPTED_FS, path, mode);
}

IOT_INLINE int __oe_remove_OE_FILE_INSECURE(const char* pathname)
{
    return oe_device_unlink(OE_DEVICE_ID_INSECURE_FS, pathname);
}

IOT_INLINE int __oe_remove_OE_FILE_SECURE_HARDWARE(const char* pathname)
{
    return oe_device_unlink(OE_DEVICE_ID_SECURE_HARDWARE_FS, pathname);
}

IOT_INLINE int __oe_remove_OE_FILE_SECURE_ENCRYPTION(const char* pathname)
{
    return oe_device_unlink(OE_DEVICE_ID_ENCRYPTED_FS, pathname);
}

IOT_INLINE OE_DIR* oe_opendir_FILE_INSECURE(const char* name)
{
    return oe_device_opendir(OE_DEVICE_ID_INSECURE_FS, name);
}

IOT_INLINE OE_DIR* oe_opendir_SECURE_HARDWARE(const char* name)
{
    return oe_device_opendir(OE_DEVICE_ID_SECURE_HARDWARE_FS, name);
}

IOT_INLINE OE_DIR* oe_opendir_SECURE_ENCRYPTION(const char* name)
{
    return oe_device_opendir(OE_DEVICE_ID_ENCRYPTED_FS, name);
}

#define oe_fclose __oe_fclose
#define oe_feof __oe_feof
#define oe_ferror __oe_ferror
#define oe_fflush __oe_fflush
#define oe_fgets __oe_fgets
#define oe_fprintf __oe_fprintf
#define oe_fputs __oe_fputs
#define oe_fread __oe_fread
#define oe_fseek __oe_fseek
#define oe_ftell __oe_ftell
#define oe_fvfprintf __oe_fvfprintf
#define oe_fwrite __oe_fwrite
#define oe_fopen_OE_FILE_INSECURE __oe_fopen_OE_FILE_INSECURE
#define oe_fopen_OE_OE_FILE_SECURE_HARDWARE \
    __oe_fopen_OE_OE_FILE_SECURE_HARDWARE
#define oe_fopen_OE_FILE_SECURE_ENCRYPTION __oe_fopen_OE_FILE_SECURE_ENCRYPTION
#define oe_remove_OE_FILE_INSECURE __oe_remove_OE_FILE_INSECURE
#define oe_remove_OE_FILE_SECURE_HARDWARE __oe_remove_OE_FILE_SECURE_HARDWARE
#define oe_remove_OE_FILE_SECURE_ENCRYPTION \
    __oe_remove_OE_FILE_SECURE_ENCRYPTION
#define oe_opendir_FILE_INSECURE __oe_opendir_FILE_INSECURE
#define oe_opendir_SECURE_HARDWARE __oe_opendir_SECURE_HARDWARE
#define oe_opendir_SECURE_ENCRYPTION __oe_opendir_SECURE_ENCRYPTION

typedef long intptr_t;

IOT_EXTERN_C_END

#endif /* _OE_IOT_STDIOBEGIN_H */
