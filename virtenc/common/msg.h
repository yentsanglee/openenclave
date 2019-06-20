// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_MSG_H
#define _VE_MSG_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

void* ve_malloc(size_t size);

void ve_free(void* ptr);

#define VE_MSG_MAGIC 0xAABBCCDD

#define __VE_MSG_MIN VE_MSG_INIT
#define __VE_MSG_MAX VE_MSG_PING_THREAD

#define VE_MSG_INITIALIZER {VE_MSG_NONE, 0, NULL};

typedef enum _ve_msg_type
{
    VE_MSG_NONE,
    VE_MSG_INIT,
    VE_MSG_TERMINATE,
    VE_MSG_ADD_THREAD,
    VE_MSG_PING_THREAD,
} ve_msg_type_t;

typedef struct _ve_msg_header
{
    uint32_t magic;
    uint32_t type;
    uint64_t size;
} ve_msg_header_t;

typedef struct _ve_msg
{
    ve_msg_type_t type;
    size_t size;
    void* data;
} ve_msg_t;

typedef struct _ve_msg_ping_thread_out
{
    int ret;
} ve_msg_ping_thread_out_t;

typedef struct _ve_msg_init_in
{
    /* Child's end of the socket pair that the parent created. */
    int sock;
} ve_msg_init_in_t;

typedef struct _ve_msg_terminate_out
{
    int ret;
} ve_msg_terminate_out_t;

typedef struct _ve_msg_add_thread_in
{
    uint32_t tcs;
    size_t stack_size;
} ve_msg_add_thread_in_t;

typedef struct _ve_msg_add_thread_out
{
    int ret;
} ve_msg_add_thread_out_t;

ssize_t ve_read(int fd, void* buf, size_t count);

ssize_t ve_write(int fd, const void* buf, size_t count);

void ve_debug(const char* str);

OE_INLINE void ve_msg_free(ve_msg_t* msg)
{
    if (msg && msg->data)
    {
        ve_free(msg->data);
        msg->data = NULL;
    }
}

OE_INLINE int ve_recv_n(int fd, void* buf, size_t count)
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

OE_INLINE int ve_send_n(int fd, const void* buf, size_t count)
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

OE_INLINE int ve_send_msg(
    int fd,
    ve_msg_type_t type,
    const void* data,
    size_t size)
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

OE_INLINE int ve_recv_msg(int fd, ve_msg_t* msg)
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

OE_INLINE int ve_recv_msg_by_type(
    int fd,
    ve_msg_type_t type,
    void* data,
    size_t size)
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

#endif /* _VE_MSG_H */
