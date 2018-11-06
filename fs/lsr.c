// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include <dirent.h>
#include "commands.h"
#include "common.h"
#include "fs.h"
#include "strarr.h"

int fs_lsr(const char* root, fs_strarr_t* paths)
{
    int ret = -1;
    DIR* dir = NULL;
    struct dirent* ent;
    char path[FS_PATH_MAX];
    fs_strarr_t dirs = FS_STRARR_INITIALIZER;

    /* Check parameters */
    if (!root || !paths)
        goto done;

    /* Open the directory */
    if (!(dir = opendir(root)))
        goto done;

    /* For each entry */
    while ((ent = readdir(dir)))
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        strlcpy(path, root, sizeof(path));

        if (strcmp(root, "/") != 0)
            strlcat(path, "/", sizeof(path));

        strlcat(path, ent->d_name, sizeof(path));

        /* Append to paths[] array */
        if (fs_strarr_append(paths, path) != 0)
            goto done;

        /* Append to dirs[] array */
        if (ent->d_type & FS_DT_DIR)
        {
            if (fs_strarr_append(&dirs, path) != 0)
                goto done;
        }
    }

    /* Recurse into child directories */
    {
        size_t i;

        for (i = 0; i < dirs.size; i++)
        {
            if (fs_lsr(dirs.data[i], paths) != 0)
            {
                goto done;
            }
        }
    }

    ret = 0;

done:

    if (dir)
        closedir(dir);

    fs_strarr_release(&dirs);

    if (ret != 0)
    {
        fs_strarr_release(paths);
        memset(paths, 0, sizeof(fs_strarr_t));
    }

    return ret;
}
