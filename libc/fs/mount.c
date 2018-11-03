// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/mount.h>
#include <stdarg.h>
#include <string.h>
#include "blockdev.h"
#include "fs.h"
#include "hostfs.h"
#include "oefs.h"
#include "trace.h"

int oe_mount_oefs(
    const char* source,
    const char* target,
    uint32_t flags,
    size_t num_blocks,
    const uint8_t key[OE_MOUNT_KEY_SIZE])
{
    int ret = -1;
    fs_block_dev_t* dev = NULL;
    fs_t* fs = NULL;

    if (!target)
        goto done;

    if (source)
    {
        /* Open a host device. */
        if (oe_open_host_block_dev(&dev, source) != 0)
            goto done;

        /* If a key was provided, then open a crypto device. */
        if (key)
        {
            fs_block_dev_t* crypto_dev = NULL;

            if (oe_open_crypto_block_dev(&crypto_dev, key, dev) != 0)
                goto done;

            dev->release(dev);
            dev = crypto_dev;
        }
    }
    else
    {
        size_t size;

        /* Open a ram device within enclave memory. */

        if (flags & OE_MOUNT_FLAG_CRYPTO)
            goto done;

        if (oefs_size(num_blocks, &size) != 0)
            goto done;

        if (oe_open_ram_block_dev(&dev, size) != 0)
            goto done;
    }

    if (flags & OE_MOUNT_FLAG_MKFS)
    {
        if (oefs_mkfs(dev, num_blocks) != 0)
            goto done;
    }

    if (oefs_initialize(&fs, dev) != 0)
        goto done;

    if (fs_bind(fs, target) != 0)
        goto done;

    fs = NULL;

    ret = 0;

done:

    if (dev)
        dev->release(dev);

    if (fs)
        fs->fs_release(fs);

    return ret;
}

int oe_mount_hostfs(const char* target)
{
    int ret = -1;
    fs_t* fs = NULL;

    if (!target)
        goto done;

    if (hostfs_initialize(&fs) != 0)
        goto done;

    if (fs_bind(fs, target) != 0)
        goto done;

    fs = NULL;

    ret = 0;

done:

    if (fs)
        fs->fs_release(fs);

    return ret;
}

int oe_unmount(const char* target)
{
    int ret = -1;

    if (!target)
        goto done;

    if (fs_unbind(target) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}
