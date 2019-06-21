// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "msg.h"

void ve_msg_free(ve_msg_t* msg)
{
    if (msg && msg->data)
    {
        ve_free(msg->data);
        msg->data = NULL;
    }
}

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

int ve_send_msg(int fd, ve_msg_type_t type, const void* data, size_t size)
{
    int ret = -1;
    ve_msg_header_t msg;

    if (size && !data)
        goto done;

    msg.magic = VE_MSG_MAGIC;
    msg.type = type;
    msg.size = size;

    if (ve_send_n(fd, &msg, sizeof(ve_msg_header_t)) != 0)
        goto done;

    if (size && ve_send_n(fd, data, size) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

int ve_recv_msg(int fd, ve_msg_t* msg)
{
    int ret = -1;
    ve_msg_header_t header;
    void* data = NULL;

    if (msg)
    {
        msg->type = VE_MSG_NONE;
        msg->data = NULL;
        msg->size = 0;
    }

    if (fd < 0 || !msg)
        goto done;

    if (ve_recv_n(fd, &header, sizeof(ve_msg_header_t)) != 0)
        goto done;

    if (header.magic != VE_MSG_MAGIC)
        goto done;

    if (!(header.type >= __VE_MSG_MIN || header.type <= __VE_MSG_MAX))
        goto done;

    if (header.size)
    {
        if (!(data = ve_malloc(header.size)))
            goto done;

        if (ve_recv_n(fd, data, header.size) != 0)
            goto done;
    }

    msg->type = header.type;
    msg->size = header.size;
    msg->data = data;
    data = NULL;

    ret = 0;

done:

    if (data)
        ve_free(data);

    return ret;
}

int ve_recv_msg_by_type(int fd, ve_msg_type_t type, void* data, size_t size)
{
    int ret = -1;
    ve_msg_header_t msg;

    if (ve_recv_n(fd, &msg, sizeof(ve_msg_header_t)) != 0)
        goto done;

    if (msg.magic != VE_MSG_MAGIC)
        goto done;

    if (msg.type != type || msg.size != size)
        goto done;

    if (ve_recv_n(fd, data, size) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}
