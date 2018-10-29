// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/calls.h>
#include <openenclave/internal/mount.h>
#include <openenclave/internal/raise.h>
#include "blockdev.h"
#include "filesys.h"

static oe_result_t _mount_oefs(const char* device_name, const char* path)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_block_dev_t* block_dev = NULL;
    oe_filesys_t* filesys = NULL;

    /* Create the host block device */
    OE_CHECK(oe_open_host_block_dev(device_name, &block_dev));

    /* Create an oefs filesys object */
    {
        if (!(filesys = oefs_new_filesys(block_dev)))
            OE_RAISE(OE_FAILURE);

        block_dev = NULL;
    }

    /* Mount the file system at the given path. */
    if (oe_filesys_mount(filesys, path) != 0)
        OE_RAISE(OE_FAILURE);

    filesys = NULL;

    result = OE_OK;

done:

    if (block_dev)
        block_dev->close(block_dev);

    if (filesys)
        filesys->release(filesys);

    return result;
}

oe_result_t oe_mount(oe_mount_type_t type, const char* device, const char* path)
{
    oe_result_t result = OE_UNEXPECTED;

    if (!device || !path)
        OE_RAISE(OE_INVALID_PARAMETER);

    switch (type)
    {
        case OE_MOUNT_TYPE_EXT2:
            OE_CHECK(_mount_oefs(device, path));
            break;
    }

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_unmount(const char* path)
{
    return OE_UNSUPPORTED;
}
