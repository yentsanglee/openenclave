// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>

int main(int argc, const char* argv[])
{
    FILE* stream;
    char tmp_dir[PATH_MAX];
    char path[PATH_MAX];

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s src-dir tmp-dir\n", argv[0]);
        exit(1);
    }

    if (mount("/", "/", "hostfs", 0, NULL) != 0)
    {
        fprintf(stderr, "mount() failed\n");
        exit(1);
    }

    /* Create the temporary directory. */
    {
        strcpy(tmp_dir, argv[2]);

        if (mkdir(tmp_dir, 0777) != 0)
        {
            fprintf(stderr, "mkdir() failed\n");
            exit(1);
        }
    }

    strcpy(path, tmp_dir);
    strcat(path, "/myfile");

    if (!(stream = fopen(path, "w")))
    {
        fprintf(stderr, "fopen() failed: %s\n", path);
        exit(1);
    }

    fclose(stream);

    exit(123);
    return 0;
}
