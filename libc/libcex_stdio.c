// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#define OE_NO_POSIX_FILE_API
#include <openenclave/libcex/stdio.h>
#define oe_opendir __oe_opendir_renamed
#define oe_readdir __oe_readdir_renamed
#define oe_closedir __oe_closedir_renamed
#define oe_remove __oe_remove_renamed
#include <openenclave/internal/fs.h>
#include <dirent.h>
// clang-format on

static oe_file_security_t _to_devid(oe_file_security_t security)
{
    switch (security)
    {
        case OE_FILE_INSECURE:
            return OE_DEVICE_ID_HOSTFS;
        case OE_FILE_SECURE_ENCRYPTION:
            return OE_DEVICE_ID_SGXFS;
        case OE_FILE_SECURE_HARDWARE:
            return OE_DEVICE_ID_SHWFS;
    }
}

int __libcex_oe_feof(OE_FILE* stream)
{
    return feof((FILE*)stream);
}

int __libcex_oe_ferror(OE_FILE* stream)
{
    return ferror((FILE*)stream);
}

int __libcex_oe_fflush(OE_FILE* stream)
{
    return fflush((FILE*)stream);
}

char* __libcex_oe_fgets(char* s, int size, OE_FILE* stream)
{
    return fgets(s, size, (FILE*)stream);
}

int __libcex_oe_fputs(const char* s, OE_FILE* stream)
{
    return fputs(s, (FILE*)stream);
}

size_t __libcex_oe_fread(void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    return fread(ptr, size, nmemb, (FILE*)stream);
}

int __libcex_oe_fseek(OE_FILE* stream, long offset, int whence)
{
    return fseek((FILE*)stream, offset, whence);
}

long __libcex_oe_ftell(OE_FILE* stream)
{
    return ftell((FILE*)stream);
}

OE_FILE* __libcex_oe_fopen(
    oe_file_security_t security,
    const char* path,
    const char* mode)
{
    oe_set_thread_default_device(_to_devid(security));
    OE_FILE* ret = (OE_FILE*)fopen(path, mode);
    oe_clear_thread_default_device();
    return ret;
}

size_t __libcex_oe_fwrite(
    const void* ptr,
    size_t size,
    size_t nmemb,
    OE_FILE* stream)
{
    return fwrite(ptr, size, nmemb, (FILE*)stream);
}

int __libcex_oe_remove(oe_file_security_t security, const char* pathname)
{
    oe_set_thread_default_device(_to_devid(security));
    int ret = oe_unlink_dev(_to_devid(security), pathname);
    oe_clear_thread_default_device();
    return ret;
}

OE_DIR* __libcex_oe_opendir(oe_file_security_t security, const char* name)
{
    oe_set_thread_default_device(_to_devid(security));
    OE_DIR* ret = (OE_DIR*)oe_opendir_dev(_to_devid(security), name);
    oe_clear_thread_default_device();
    return ret;
}

int __libcex_oe_fclose(OE_FILE* stream)
{
    return fclose((FILE*)stream);
}

int __libcex_oe_closedir(OE_DIR* dirp)
{
    return closedir((DIR*)dirp);
}
