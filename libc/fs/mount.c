// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "mount.h"
#include <stdarg.h>
#include <string.h>
#include "blockdev.h"
#include "fs.h"
#include "hostfs.h"
#include "oefs.h"

#define USE_CACHE

int fs_mount_oefs(
    const char* source,
    const char* target,
    uint32_t flags,
    size_t num_blocks,
    const uint8_t key[FS_MOUNT_KEY_SIZE])
{
    int ret = -1;
    fs_block_dev_t* host_dev = NULL;
    fs_block_dev_t* crypto_dev = NULL;
    fs_block_dev_t* cache_dev = NULL;
    fs_block_dev_t* ram_dev = NULL;
    fs_block_dev_t* dev = NULL;
    fs_t* fs = NULL;

    if (!target)
        goto done;

    if (source)
    {
        /* Open a host device. */
        if (fs_open_host_block_dev(&host_dev, source) != 0)
            goto done;

        /* If a key was provided, then open a crypto device. */
        if (key)
        {
            if (fs_open_crypto_block_dev(&crypto_dev, key, host_dev) != 0)
                goto done;

#if defined(USE_CACHE)
            if (fs_open_cache_block_dev(&cache_dev, crypto_dev) != 0)
                goto done;

            dev = cache_dev;
#else
            dev = crypto_dev;
#endif
        }
        else
        {
            dev = host_dev;
        }
    }
    else
    {
        size_t size;

        /* Open a ram device within enclave memory. */

        if (flags & FS_MOUNT_FLAG_CRYPTO)
            goto done;

        if (oefs_size(num_blocks, &size) != 0)
            goto done;

        if (fs_open_ram_block_dev(&ram_dev, size) != 0)
            goto done;

        dev = ram_dev;
    }

    if (flags & FS_MOUNT_FLAG_MKFS)
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

    if (host_dev)
        host_dev->release(host_dev);

    if (crypto_dev)
        crypto_dev->release(crypto_dev);

    if (cache_dev)
        cache_dev->release(cache_dev);

    if (ram_dev)
        ram_dev->release(ram_dev);

    if (fs)
        fs->fs_release(fs);

    return ret;
}

int fs_mount_hostfs(const char* target)
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

int fs_unmount(const char* target)
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
