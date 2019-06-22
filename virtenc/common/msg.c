// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "msg.h"

int ve_recv_n(int fd, void* buf, size_t count)
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
        n -= r;
    }

    ret = 0;

done:
    return ret;
}

int ve_send_n(int fd, const void* buf, size_t count)
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
        n -= r;
    }

    ret = 0;

done:
    return ret;
}

const char* ve_func_name(ve_func_t func)
{
    switch (func)
    {
        case VE_FUNC_RET:
            return "RET";
        case VE_FUNC_ERR:
            return "ERR";
        case VE_FUNC_INIT:
            return "INIT";
        case VE_FUNC_ADD_THREAD:
            return "ADD_THREAD";
        case VE_FUNC_TERMINATE_THREAD:
            return "TERMINATE_THREAD";
        case VE_FUNC_TERMINATE:
            return "TERMINATE";
        case VE_FUNC_PING:
            return "PING";
    }

    return "UNKNOWN";
}
