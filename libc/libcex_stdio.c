// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <errno.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/stdioex.h>
#include <openenclave/internal/fs.h>
#define _FILE_DEFINED
#include <openenclave/iot/stdiov2.h>
// clang-format on

static oe_file_security_t _file_security_to_devid(oe_file_security_t security)
{
    switch (security)
    {
        case OE_FILE_INSECURE:
            return 3; /* OE_DEVICE_ID_HOSTFS */
        case OE_FILE_SECURE_ENCRYPTION:
            return 4; /* OE_DEVICE_ID_SGXFS */
        case OE_FILE_SECURE_HARDWARE:
            return 5; /* OE_DEVICE_ID_SHWFS */
    }
}

OE_FILE *oe_fopen(
    oe_file_security_t security,
    const char *path,
    const char *mode)
{
    return oe_fopen_dev(_file_security_to_devid(security), path, mode);
}

int oe_remove(oe_file_security_t security, const char *pathname)
{
    return oe_unlink_dev(_file_security_to_devid(security), pathname);
}
