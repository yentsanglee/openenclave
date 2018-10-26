#include "filesys.h"
#include "oefs.h"
#include <string.h>

typedef struct _file_impl
{
    oe_file_t base;
    oefs_file_t* oefs_file;
} file_impl_t;

static ssize_t _file_read(oe_file_t* file, void* buf, size_t count)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;

    if (!file_impl || !file_impl->oefs_file)
        goto done;

    ret = oefs_read_file(file_impl->oefs_file, buf, count);

done:
    return ret;
}

static ssize_t _file_write(oe_file_t* file, const void* buf, size_t count)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;

    if (!file_impl || !file_impl->oefs_file)
        goto done;

    ret = oefs_write_file(file_impl->oefs_file, buf, count);

done:
    return ret;
}

static int _file_close(oe_file_t* file)
{
    ssize_t ret = -1;
    file_impl_t* file_impl = (file_impl_t*)file;

    if (!file_impl || !file_impl->oefs_file)
        goto done;

    if (oefs_close_file(file_impl->oefs_file) != OEFS_OK)
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
    oefs_file_t* oefs_file = NULL;

    if (!filesys_impl || !filesys_impl->oefs)
        goto done;

    /* ATTN: support create mode. */

#if 0
    if (!(oefs_file = oefs_open_file(filesys_impl->oefs, path, mode)))
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
        oefs_close_file(oefs_file);

    return ret;
}

static int _filesys_release(oe_filesys_t* filesys)
{
    filesys_impl_t* filesys_impl = (filesys_impl_t*)filesys;
    int ret = -1;

    if (!filesys_impl || !filesys_impl->oefs)
        goto done;

    oefs_delete(filesys_impl->oefs);

    free(filesys_impl);

done:
    return ret;
}

oe_filesys_t* oefs_new_filesys(oe_block_dev_t* dev)
{
    oe_filesys_t* ret = NULL;
    oefs_t* oefs = NULL;

    if (oefs_new(&oefs, dev) != OEFS_OK)
        goto done;

    {
        filesys_impl_t* filesys_impl = NULL;

        if (!(filesys_impl = calloc(1, sizeof(filesys_impl_t))))
            goto done;

        filesys_impl->base.open_file = _filesys_open_file;
        filesys_impl->base.release = _filesys_release;
        filesys_impl->oefs = oefs;
        oefs = NULL;

        ret = &filesys_impl->base;
    }

done:

    if (oefs)
        oefs_delete(oefs);

    return ret;
}
