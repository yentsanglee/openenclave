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

#define MAX_CALLBACKS 16

static int _mount_oefs_callback(
    const char* type,
    const char* source,
    const char* target,
    va_list ap)
{
    uint32_t flags = va_arg(ap, uint32_t);
    size_t nblks = va_arg(ap, size_t);
    const uint8_t* key = va_arg(ap, const uint8_t*);

    return fs_mount_oefs(source, target, flags, nblks, key);
}

static int _mount_hostfs_callback(
    const char* type, 
    const char* source, 
    const char* target, 
    va_list ap)
{
    return fs_mount_hostfs(source, target);
}

typedef struct _callback
{
    const char* type;
    fs_mount_callback_t callback;
}
callback_t;

static callback_t _callbacks[MAX_CALLBACKS] =
{
    { "hostfs", _mount_hostfs_callback},
    { "oefs", _mount_oefs_callback },
};

static size_t _ncallbacks = 2;

int fs_register(const char* type, fs_mount_callback_t callback)
{
    int ret = -1;

    if (!callback)
        goto done;

    if (_ncallbacks == MAX_CALLBACKS)
        goto done;

    _callbacks[_ncallbacks].type = type;
    _callbacks[_ncallbacks].callback = callback;
    _ncallbacks++;

done:
    return ret;
}

int fs_mount(const char* type, const char* source, const char* target, ...)
{
    int ret = -1;

    if (!type)
        goto done;

    for (size_t i = 0; i < _ncallbacks; i++)
    {
        if (strcmp(_callbacks[i].type, type) == 0)
        {
            va_list ap;
            va_start(ap, target);
            ret = (*_callbacks[i].callback)(type, source, target, ap);
            va_end(ap);
            goto done;
        }
    }

    /* Not found. */

done:
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
