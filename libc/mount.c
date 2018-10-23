#include <openenclave/internal/mount.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/raise.h>
#include "filesys.h"
#include "blockdevice.h"
#include "ext2.h"

static oe_result_t _mount_ext2(const char* device_name, const char* path)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_block_device_t* block_device = NULL;
    oe_filesys_t* filesys = NULL;

    /* Create the host block device */
    OE_CHECK(oe_open_block_device(device_name, &block_device));

    /* Create an ext2 filesys object */
    {
        if (!(filesys = ext2_new_filesys(block_device)))
            OE_RAISE(OE_FAILURE);

        block_device = NULL;
    }

    /* Mount the file system at the given path. */
    if (oe_filesys_mount(filesys, path) != 0)
        OE_RAISE(OE_FAILURE);

    filesys = NULL;

    result = OE_OK;

done:

    if (block_device)
        block_device->close(block_device);

    if (filesys)
        filesys->release(filesys);

    return result;
}

oe_result_t oe_mount(
    oe_mount_type_t type,
    const char* device,
    const char* path)
{
    oe_result_t result = OE_UNEXPECTED;

    if (!device || !path)
        OE_RAISE(OE_INVALID_PARAMETER);

    switch(type)
    {
        case OE_MOUNT_TYPE_EXT2:
            OE_CHECK(_mount_ext2(device, path));
            break;
    }

done:
    return result;
}

oe_result_t oe_unmount(
    const char* path)
{
    return OE_UNSUPPORTED;
}
