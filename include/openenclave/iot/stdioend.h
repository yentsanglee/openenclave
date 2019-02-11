/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _OE_IOT_STDIOEND_H
#define _OE_IOT_STDIOEND_H

#if defined(_STDIOBEGIN_DEFINED_OE_ENCLAVE_H)
#undef _OE_ENCLAVE_H
#endif

#undef oe_fclose
#undef oe_feof
#undef oe_ferror
#undef oe_fflush
#undef oe_fgets
#undef oe_fprintf
#undef oe_fputs
#undef oe_fread
#undef oe_fseek
#undef oe_ftell
#undef oe_vfprintf
#undef oe_fwrite
#undef oe_fopen_OE_FILE_INSECURE
#undef oe_fopen_OE_FILE_SECURE_HARDWARE
#undef oe_fopen_OE_FILE_SECURE_ENCRYPTION
#undef oe_remove_OE_FILE_INSECURE
#undef oe_remove_OE_FILE_SECURE_HARDWARE
#undef oe_remove_OE_FILE_SECURE_ENCRYPTION
#undef oe_opendir_FILE_INSECURE
#undef oe_opendir_SECURE_HARDWARE
#undef oe_opendir_SECURE_ENCRYPTION

IOT_INLINE int oe_fclose(OE_FILE* stream)
{
    return __oe_fclose(stream);
}

IOT_INLINE int oe_feof(OE_FILE* stream)
{
    return __oe_feof(stream);
}

IOT_INLINE int oe_ferror(OE_FILE* stream)
{
    return __oe_ferror(stream);
}

IOT_INLINE int oe_fflush(OE_FILE* stream)
{
    return __oe_fflush(stream);
}

IOT_INLINE char* oe_fgets(char* s, int size, OE_FILE* stream)
{
    return __oe_fgets(s, size, stream);
}

IOT_INLINE int oe_fprintf(
    OE_FILE* const stream,
    char const* const format,
    ...)
{
    va_list ap;
    va_start(ap, format);
    int n = __oe_vfprintf(stream, format, ap);
    va_end(ap);
    return n;
}

IOT_INLINE int oe_fputs(const char* s, OE_FILE* stream)
{
    return __oe_fputs(s, stream);
}

IOT_INLINE size_t
oe_fread(void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    return __oe_fread(ptr, size, nmemb, stream);
}

IOT_INLINE int oe_fseek(OE_FILE* stream, long offset, int whence)
{
    return __oe_fseek(stream, offset, whence);
}

IOT_INLINE long oe_ftell(OE_FILE* stream)
{
    return __oe_ftell(stream);
}

IOT_INLINE int oe_vfprintf(
    OE_FILE* stream,
    const char* format,
    va_list argptr)
{
    return __oe_vfprintf(stream, format, argptr);
}

IOT_INLINE size_t
oe_fwrite(const void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    return __oe_fwrite(ptr, size, nmemb, stream);
}

IOT_INLINE OE_FILE* oe_fopen_OE_FILE_INSECURE(
    const char* path,
    const char* mode)
{
    return oe_device_fopen(OE_DEVICE_ID_INSECURE_FS, path, mode);
}

IOT_INLINE OE_FILE* oe_fopen_OE_FILE_SECURE_HARDWARE(
    const char* path,
    const char* mode)
{
    return oe_device_fopen(OE_DEVICE_ID_SECURE_HARDWARE_FS, path, mode);
}

IOT_INLINE OE_FILE* oe_fopen_OE_FILE_SECURE_ENCRYPTION(
    const char* path,
    const char* mode)
{
    return oe_device_fopen(OE_DEVICE_ID_ENCRYPTED_FS, path, mode);
}

IOT_INLINE int oe_remove_OE_FILE_INSECURE(const char* pathname)
{
    return oe_device_unlink(OE_DEVICE_ID_INSECURE_FS, pathname);
}

IOT_INLINE int oe_remove_OE_FILE_SECURE_HARDWARE(const char* pathname)
{
    return oe_device_unlink(OE_DEVICE_ID_SECURE_HARDWARE_FS, pathname);
}

IOT_INLINE int oe_remove_OE_FILE_SECURE_ENCRYPTION(const char* pathname)
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

#endif /* _OE_IOT_STDIOEND_H */
