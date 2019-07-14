// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "io.h"
#include "trace.h"

int ve_readn(int fd, void* buf, size_t count)
{
    int ret = -1;
    uint8_t* p = (uint8_t*)buf;
    size_t n = count;

    if (fd < 0 || !buf)
        goto done;

    while (n > 0)
    {
        ssize_t r = ve_read(fd, p, n);

        if (r <= 0)
            goto done;

        p += r;
        n -= (size_t)r;
    }

    ret = 0;

done:
    return ret;
}

int ve_writen(int fd, const void* buf, size_t count)
{
    int ret = -1;
    const uint8_t* p = (const uint8_t*)buf;
    size_t n = count;

    if (fd < 0 || !buf)
        goto done;

    while (n > 0)
    {
        ssize_t r = ve_write(fd, p, n);

        if (r <= 0)
            goto done;

        p += r;
        n -= (size_t)r;
    }

    ret = 0;

done:
    return ret;
}
