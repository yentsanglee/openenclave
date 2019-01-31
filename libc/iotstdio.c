// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <errno.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/stdioex.h>
#include <openenclave/internal/fs.h>
#define _FILE_DEFINED
#include <openenclave/iot/stdio.h>
// clang-format on

OE_FILE *oe_fopen_OE_FILE_INSECURE(const char* path, const char* mode)
{
    return oe_fopen_dev(OE_DEVICE_ID_HOSTFS, path, mode);
}

OE_FILE *oe_fopen_OE_FILE_SECURE_HARDWARE(const char* path, const char* mode)
{
    /* Not supported on SGX. */
    errno = EINVAL;
    return NULL;
}

OE_FILE *oe_fopen_OE_FILE_SECURE_ENCRYPTION(const char* path, const char* mode)
{
    return oe_fopen_dev(OE_DEVICE_ID_SGXFS, path, mode);
}

int oe_remove_OE_FILE_INSECURE(const char *pathname)
{
    return oe_unlink_dev(OE_DEVICE_ID_HOSTFS, pathname);
}

int oe_remove_OE_FILE_SECURE_HARDWARE(const char *pathname)
{
    /* Not supported on SGX. */
    errno = EINVAL;
    return -1;
}

int oe_remove_OE_FILE_SECURE_ENCRYPTION(const char *pathname)
{
    return oe_unlink_dev(OE_DEVICE_ID_SGXFS, pathname);
}

OE_DIR *oe_opendir_FILE_INSECURE(const char *name)
{
    return (OE_DIR*)oe_opendir_dev(OE_DEVICE_ID_HOSTFS, name);
}

OE_DIR *oe_opendir_SECURE_HARDWARE(const char *name)
{
    errno = EINVAL;
    return NULL;
}

OE_DIR *oe_opendir_SECURE_ENCRYPTION(const char *name)
{
    return (OE_DIR*)oe_opendir_dev(OE_DEVICE_ID_SGXFS, name);
}
