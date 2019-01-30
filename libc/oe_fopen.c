// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <errno.h>

FILE *oe_fopen(int device_id, const char *path, const char *mode)
{
    FILE* ret = NULL;

    if (device_id < 0 || !path || !mode)
    {
        errno = EINVAL;
        goto done;
    }

done:
    return ret;
}
