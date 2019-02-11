// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#define OE_NO_POSIX_FILE_API
#include <openenclave/iot/stdio.h>
#include <openenclave/bits/device.h>
#include <dirent.h>
// clang-format on

int oe_set_thread_default_device(int devid);
int oe_clear_thread_default_device();

static oe_file_security_t _to_devid(oe_file_security_t security)
{
    switch (security)
    {
        case OE_FILE_INSECURE:
            return OE_DEVICE_ID_INSECURE_FS;
        case OE_FILE_SECURE_ENCRYPTION:
            return OE_DEVICE_ID_ENCRYPTED_FS;
        case OE_FILE_SECURE_HARDWARE:
            return OE_DEVICE_ID_SECURE_HARDWARE_FS;
    }
}

int iot_oe_feof(OE_FILE* stream)
{
    return feof((FILE*)stream);
}

int iot_oe_ferror(OE_FILE* stream)
{
    return ferror((FILE*)stream);
}

int iot_oe_fflush(OE_FILE* stream)
{
    return fflush((FILE*)stream);
}

char* iot_oe_fgets(char* s, int size, OE_FILE* stream)
{
    return fgets(s, size, (FILE*)stream);
}

int iot_oe_fputs(const char* s, OE_FILE* stream)
{
    return fputs(s, (FILE*)stream);
}

size_t iot_oe_fread(void* ptr, size_t size, size_t nmemb, OE_FILE* stream)
{
    return fread(ptr, size, nmemb, (FILE*)stream);
}

int iot_oe_fseek(OE_FILE* stream, long offset, int whence)
{
    return fseek((FILE*)stream, offset, whence);
}

long iot_oe_ftell(OE_FILE* stream)
{
    return ftell((FILE*)stream);
}

OE_FILE* iot_oe_fopen(
    oe_file_security_t security,
    const char* path,
    const char* mode)
{
    oe_set_thread_default_device(_to_devid(security));
    OE_FILE* ret = (OE_FILE*)fopen(path, mode);
    oe_clear_thread_default_device();
    return ret;
}

size_t iot_oe_fwrite(
    const void* ptr,
    size_t size,
    size_t nmemb,
    OE_FILE* stream)
{
    return fwrite(ptr, size, nmemb, (FILE*)stream);
}

int iot_oe_remove(oe_file_security_t security, const char* pathname)
{
    oe_set_thread_default_device(_to_devid(security));
    int ret = oe_device_unlink(_to_devid(security), pathname);
    oe_clear_thread_default_device();
    return ret;
}

OE_DIR* iot_oe_opendir(oe_file_security_t security, const char* name)
{
    oe_set_thread_default_device(_to_devid(security));
    OE_DIR* ret = (OE_DIR*)oe_device_opendir(_to_devid(security), name);
    oe_clear_thread_default_device();
    return ret;
}

int iot_oe_fclose(OE_FILE* stream)
{
    return fclose((FILE*)stream);
}

int iot_oe_closedir(OE_DIR* dirp)
{
    return closedir((DIR*)dirp);
}
