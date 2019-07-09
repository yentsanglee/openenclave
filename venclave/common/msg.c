// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "msg.h"
#include "io.h"

int ve_msg_send(int fd, ve_msg_type_t type, const void* data, size_t size)
{
    int ret = -1;
    ve_msg_t* msg = NULL;
    size_t msg_size;

    if (fd < 0 || (size && !data))
        goto done;

    msg_size = sizeof(ve_msg_t) + size;

    if (!(msg = ve_calloc(1, msg_size)))
        goto done;

    msg->magic = VE_MSG_MAGIC;
    msg->type = type;
    msg->size = size;

    if (size)
        memcpy(msg->data, data, size);

    if (ve_writen(fd, msg, msg_size) != 0)
        goto done;

    ret = 0;

done:

    if (msg)
        ve_free(msg);

    return ret;
}

int ve_msg_recv(int fd, ve_msg_type_t type, void* data, size_t size)
{
    int ret = -1;
    ve_msg_t msg;

    if (fd < 0 || (size && !data))
        goto done;

    if (ve_readn(fd, &msg, sizeof(msg)) != 0)
        goto done;

    if (msg.magic != VE_MSG_MAGIC)
        goto done;

    if (msg.type != type)
        goto done;

    if (msg.size != size)
        goto done;

    if (msg.size)
    {
        if (ve_readn(fd, data, msg.size) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

int ve_msg_recv_any(int fd, ve_msg_type_t* type, void** data, size_t* size)
{
    int ret = -1;
    ve_msg_t msg;
    void* ptr = NULL;

    if (data)
        *data = NULL;

    if (size)
        *size = 0;

    if (fd < 0 || !type || !data || !size)
        goto done;

    if (ve_readn(fd, &msg, sizeof(msg)) != 0)
        goto done;

    if (msg.magic != VE_MSG_MAGIC)
        goto done;

    if (msg.size)
    {
        if (!(ptr = ve_calloc(1, msg.size)))
            goto done;

        if (ve_readn(fd, ptr, msg.size) != 0)
            goto done;
    }

    *type = (ve_msg_type_t)msg.type;
    *size = msg.size;
    *data = ptr;
    ptr = NULL;

    ret = 0;

done:

    if (ptr)
        ve_free(ptr);

    return ret;
}
