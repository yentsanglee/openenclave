// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "mount.h"
#include <stdarg.h>
#include <string.h>
#include "blkdev.h"
#include "fs.h"
#include "hostfs.h"
#include "oefs.h"

#define USE_CACHE_BLKDEV
#define USE_MERKLE_BLKDEV
#define USE_CRYPTO_BLKDEV

FS_INLINE bool _is_power_of_two(size_t n)
{
    return (n & (n - 1)) == 0;
}

int fs_mount_oefs(
    const char* source,
    const char* target,
    uint32_t flags,
    size_t num_blocks,
    const uint8_t key[FS_MOUNT_KEY_SIZE])
{
    int ret = -1;
    fs_blkdev_t* host_dev = NULL;
    fs_blkdev_t* crypto_dev = NULL;
    fs_blkdev_t* cache_dev = NULL;
    fs_blkdev_t* ram_dev = NULL;
    fs_blkdev_t* merkle_dev = NULL;
    fs_blkdev_t* dev = NULL;
    fs_blkdev_t* next = NULL;
    fs_t* fs = NULL;

    if (!target)
        goto done;

    if (num_blocks < 2 || !_is_power_of_two(num_blocks))
        goto done;

    if (source)
    {
        /* Open a host device. */
        if (fs_open_host_blkdev(&host_dev, source) != 0)
            goto done;

        next = host_dev;

        /* If a key was provided, then open a crypto device. */
        if (key)
        {
#if defined(USE_CRYPTO_BLKDEV)
            {
                if (fs_open_crypto_blkdev(&crypto_dev, key, next) != 0)
                    goto done;

                next = crypto_dev;
            }
#endif

#if defined(USE_MERKLE_BLKDEV)
            {
                bool initialize = (flags & FS_MOUNT_FLAG_MKFS);

                if (fs_open_merkle_blkdev(
                    &merkle_dev, num_blocks, initialize, next) != 0)
                {
                    goto done;
                }

                next = merkle_dev;
            }
#endif

#if defined(USE_CACHE_BLKDEV)
            /* cache_dev */
            {
                if (fs_open_cache_blkdev(&cache_dev, next) != 0)
                    goto done;

                next = cache_dev;
            }
#endif
        }

        dev = next;
    }
    else
    {
        size_t size;

        /* Open a ram device within enclave memory. */

        if (flags & FS_MOUNT_FLAG_CRYPTO)
            goto done;

        if (oefs_size(num_blocks, &size) != 0)
            goto done;

        if (fs_open_ram_blkdev(&ram_dev, size) != 0)
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

    if (merkle_dev)
        merkle_dev->release(merkle_dev);

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
