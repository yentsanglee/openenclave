#ifndef _VE_MSG_H
#define _VE_MSG_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

#define VE_MSG_MAGIC 0xAABBCCDD

typedef enum _ve_msg_type
{
    VE_MSG_INIT,
    VE_MSG_PRINT,
    VE_MSG_TERMINATE,
    VE_MSG_NEW_THREAD,
} ve_msg_type_t;

#define __VE_MSG_MIN VE_MSG_INIT
#define __VE_MSG_MAX VE_MSG_NEW_THREAD

typedef struct _ve_msg
{
    uint32_t magic;
    uint32_t type;
    uint64_t size;
} ve_msg_t;

typedef struct _ve_msg_print_in
{
    uint8_t data[0];
} ve_msg_print_in_t;

typedef struct _ve_msg_print_out
{
    int ret;
} ve_msg_print_out_t;

typedef struct _ve_msg_init_in
{
    /* Child's end of the socket pair that the parent created. */
    int sock;
} ve_msg_init_in_t;

typedef struct _ve_msg_init_out
{
    int ret;
} ve_msg_init_out_t;

typedef struct _ve_msg_terminate_in
{
    int status;
} ve_msg_terminate_in_t;

typedef struct _ve_msg_terminate_out
{
    int ret;
} ve_msg_terminate_out_t;

typedef struct _ve_msg_new_thread_in
{
    uint32_t tcs;
} ve_msg_new_thread_in_t;

typedef struct _ve_msg_new_thread_out
{
    int ret;
} ve_msg_new_thread_out_t;

ssize_t ve_read(int fd, void* buf, size_t count);

ssize_t ve_write(int fd, const void* buf, size_t count);

void ve_debug(const char* str);

OE_INLINE int ve_recv_n(int fd, void* buf, size_t count, bool* eof)
{
    int ret = -1;
    uint8_t* p = (uint8_t*)buf;
    size_t n = count;

    if (eof)
        *eof = false;

    if (fd < 0 || !buf || !eof)
        goto done;

    while (n > 0)
    {
        ssize_t r = ve_read(fd, p, n);

        if (r == 0)
        {
            *eof = true;
            goto done;
        }

        if (r < 0)
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
    ve_msg_t msg;

    if (size && !data)
        goto done;

    msg.magic = VE_MSG_MAGIC;
    msg.type = type;
    msg.size = size;

    if (ve_send_n(fd, &msg, sizeof(ve_msg_t)) != 0)
        goto done;

    if (size && ve_send_n(fd, data, size) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

OE_INLINE int ve_recv_msg(int fd, ve_msg_type_t* type, size_t* size, bool* eof)
{
    int ret = -1;
    ve_msg_t msg;

    if (eof)
        *eof = false;

    if (!type || !size || !eof)
        goto done;

    if (ve_recv_n(fd, &msg, sizeof(ve_msg_t), eof) != 0)
        goto done;

    if (msg.magic != VE_MSG_MAGIC)
        goto done;

    if (!(msg.type >= __VE_MSG_MIN || msg.type <= __VE_MSG_MAX))
        goto done;

    *type = msg.type;
    *size = msg.size;

    ret = 0;

done:
    return ret;
}

OE_INLINE int ve_recv_msg_by_type(
    int fd,
    ve_msg_type_t type,
    void* data,
    size_t size,
    bool* eof)
{
    int ret = -1;
    ve_msg_t msg;

    if (eof)
        *eof = false;

    if (!eof)
        goto done;

    if (ve_recv_n(fd, &msg, sizeof(ve_msg_t), eof) != 0)
        goto done;

    if (msg.magic != VE_MSG_MAGIC)
        goto done;

    if (msg.type != type || msg.size != size)
        goto done;

    if (ve_recv_n(fd, data, size, eof) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

#endif /* _VE_MSG_H */
