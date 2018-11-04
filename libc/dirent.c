// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "fs/fs.h"

DIR* opendir(const char* name)
{
    fs_errno_t err;
    DIR* dirp;

    if ((err = fs_opendir(name, &dirp)) != 0)
    {
        errno = err;
        return NULL;
    }

    return dirp;
}

struct dirent* readdir(DIR* dirp)
{
    fs_errno_t err;
    struct dirent* entry;

    if ((err = fs_readdir(dirp, &entry)) != 0)
    {
        errno = err;
        return NULL;
    }

    return entry;
}

int readdir_r(DIR* dirp, struct dirent* entry_out, struct dirent** result_out)
{
    fs_errno_t err;
    struct dirent* entry;

    if (entry_out)
        memset(entry_out, 0, sizeof(struct dirent));

    if (result_out)
        *result_out = NULL;

    if (!dirp || !entry_out || !result_out)
    {
        errno = EINVAL;
        return -1;
    }

    if ((err = fs_readdir(dirp, &entry)) != 0)
    {
        errno = err;
        return -1;
    }

    if (!entry)
        return 0;

    memcpy(entry_out, entry, sizeof(struct dirent));
    *result_out = entry_out;

    return 0;
}

int closedir(DIR* dirp)
{
    fs_errno_t err;
    int ret;

    if ((err = fs_closedir(dirp, &ret)) != 0)
    {
        errno = err;
        return -1;
    }

    return ret;
}
