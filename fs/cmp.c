// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include "commands.h"
#include <sys/stat.h>

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
