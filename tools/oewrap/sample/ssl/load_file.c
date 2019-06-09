// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int load_file(const char* path, void** data_out, size_t* size_out)
{
    int ret = -1;
    FILE* is = NULL;
    void* data = NULL;
    size_t size = 0;

    if (data_out)
        *data_out = NULL;

    if (size_out)
        *size_out = 0;

    /* Check parameters */
    if (!path || !data_out || !size_out)
        goto done;

    /* Get size of this file */
    {
        struct stat st;

        if (stat(path, &st) != 0)
            goto done;

        size = (size_t)st.st_size;
    }

    /* Allocate memory */
    if (!(data = malloc(size)))
        goto done;

    /* Open the file */
    if (!(is = fopen(path, "r")))
        goto done;

    /* Read file into memory */
    if (fread(data, 1, size, is) != size)
        goto done;

    *data_out = data;
    *size_out = size;
    data = NULL;
    ret = 0;

done:

    if (data)
        free(data);

    if (is)
        fclose(is);

    return ret;
}
