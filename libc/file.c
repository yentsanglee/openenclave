#include <openenclave/internal/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "filesys.h"

oe_file_t* oe_file_open(const char* path, int flags, mode_t mode)
{
    oe_filesys_t* fs = NULL;
    oe_file_t* ret = NULL;
    oe_file_t* file = NULL;
    char fs_path[PATH_MAX];

    if (!path)
        goto done;

    if (!(fs = oe_filesys_lookup(path, fs_path)))
        goto done;

    if (!(file = fs->open_file(fs, fs_path, flags, mode)))
        goto done;

    ret = file;

done:
    return ret;
}

ssize_t oe_file_read(oe_file_t* file, void *buf, size_t count)
{
    ssize_t ret = -1;

    if (!file)
        goto done;

    ret = file->read(file, buf, count);

done:
    return ret;
}

ssize_t oe_file_write(oe_file_t* file, const void *buf, size_t count)
{
    ssize_t ret = -1;

    if (!file)
        goto done;

    ret = file->write(file, buf, count);

done:
    return ret;
}

int oe_file_close(oe_file_t* file)
{
    int ret = -1;

    if (!file)
        goto done;

    ret = file->close(file);

done:
    return ret;
}
