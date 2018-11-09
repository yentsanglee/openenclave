// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include "commands.h"
#include <dirent.h>
#include <sys/stat.h>
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
                goto done;
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

int fs_cmp(const char* path1, const char* path2)
{
    int ret = -1;
    struct stat st1;
    struct stat st2;
    char buf1[512];
    char buf2[512];
    FILE* is1 = NULL;
    FILE* is2 = NULL;
    size_t size = 0;

    if (!path1 || !path2)
        goto done;

    if (stat(path1, &st1) != 0)
        goto done;

    if (stat(path2, &st2) != 0)
        goto done;

    if (S_ISDIR(st1.st_mode) && !S_ISDIR(st2.st_mode))
        goto done;

    if (!S_ISDIR(st1.st_mode) && S_ISDIR(st2.st_mode))
        goto done;

    if (S_ISREG(st1.st_mode) && !S_ISREG(st2.st_mode))
        goto done;

    if (!S_ISREG(st1.st_mode) && S_ISREG(st2.st_mode))
        goto done;

    if (S_ISDIR(st1.st_mode))
    {
        ret = 0;
        goto done;
    }

    if (st1.st_size != st2.st_size)
        goto done;

    if (!(is1 = fopen(path1, "rb")))
        goto done;

    if (!(is2 = fopen(path2, "rb")))
        goto done;

    for (;;)
    {
        ssize_t n1 = fread(buf1, 1, sizeof(buf1), is1);
        ssize_t n2 = fread(buf2, 1, sizeof(buf2), is2);

        if (n1 != n2)
            goto done;

        if (n1 <= 0)
            break;

        if (memcmp(buf1, buf2, n1) != 0)
            goto done;

        size += n1;
    }

    if (size != st1.st_size)
        goto done;

    ret = 0;

done:

    if (is1)
        fclose(is1);

    if (is2)
        fclose(is2);

    return ret;
}
