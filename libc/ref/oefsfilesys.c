// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "oefs.h"
#include <string.h>
#include "filesys.h"

typedef struct _oefs oefs_t;

typedef struct _file_impl
{
    oe_file_t base;
    fs_file_t* oefs_file;
} file_impl_t;

static ssize_t _file_read(oe_file_t* file, void* buf, size_t count)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;
    int32_t n;

    if (!file_impl || !file_impl->oefs_file)
        goto done;

    if (oefs_read(file_impl->oefs_file, buf, count, &n) != 0)
        goto done;

    ret = n;

done:
    return ret;
}

static ssize_t _file_write(oe_file_t* file, const void* buf, size_t count)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;
    int32_t nwritten;

    if (!file_impl || !file_impl->oefs_file)
        goto done;

    if (oefs_write(file_impl->oefs_file, buf, count, &nwritten) != OE_EOK)
        goto done;

    ret = nwritten;

done:
    return ret;
}

static int _file_close(oe_file_t* file)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;

    if (!file_impl || !file_impl->oefs_file)
        goto done;

    if (oefs_close(file_impl->oefs_file) != OE_EOK)
        goto done;

    memset(file_impl, 0, sizeof(file_impl_t));
    free(file_impl);
    ret = 0;

done:
    return ret;
}

typedef struct _filesys_impl
{
    oe_filesys_t base;
    oefs_t* oefs;
} filesys_impl_t;

static oe_file_t* _filesys_open_file(
    oe_filesys_t* filesys,
    const char* path,
    int flags,
    mode_t mode)
{
    oe_file_t* ret = NULL;
    filesys_impl_t* filesys_impl = (filesys_impl_t*)filesys;
    fs_file_t* oefs_file = NULL;

    if (!filesys_impl || !filesys_impl->oefs)
        goto done;

/* ATTN: support create mode. */

#if 0
    if (!(oefs_file = oefs_open(filesys_impl->oefs, path, mode)))
        goto done;
#else
    goto done;
#endif

    /* Allocate and initialize the file struct. */
    {
        file_impl_t* file_impl;

        if (!(file_impl = calloc(1, sizeof(file_impl_t))))
            goto done;

        file_impl->base.read = _file_read;
        file_impl->base.write = _file_write;
        file_impl->base.close = _file_close;
        file_impl->oefs_file = oefs_file;
        oefs_file = NULL;

        ret = &file_impl->base;
    }

done:

    if (oefs_file)
        oefs_close(oefs_file);

    return ret;
}

static int _filesys_release(oe_filesys_t* filesys)
{
    filesys_impl_t* filesys_impl = (filesys_impl_t*)filesys;
    int ret = -1;

    if (!filesys_impl || !filesys_impl->oefs)
        goto done;

    oefs_release(&filesys_impl->oefs->base);

    free(filesys_impl);

done:
    return ret;
}

oe_filesys_t* oefs_new_filesys(oe_block_dev_t* dev)
{
    oe_filesys_t* ret = NULL;
    fs_t* fs = NULL;

    if (oefs_initialize(&fs, dev) != OE_EOK)
        goto done;

    {
        filesys_impl_t* filesys_impl = NULL;

        if (!(filesys_impl = calloc(1, sizeof(filesys_impl_t))))
            goto done;

        filesys_impl->base.open_file = _filesys_open_file;
        filesys_impl->base.release = _filesys_release;
        filesys_impl->oefs = (oefs_t*)fs;
        fs = NULL;

        ret = &filesys_impl->base;
    }

done:

    if (fs)
        oefs_release(fs);

    return ret;
}
