// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_CALL_H
#define _VE_CALL_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

#define VE_INIT_ARG_MAGIC 0x3b146763853147ce

#define VE_SHMADDR_MAGIC 0xc500f4b6cd2c4f42

typedef enum _ve_func
{
    VE_FUNC_RET,
    VE_FUNC_ERR,
    VE_FUNC_INIT,
    VE_FUNC_PING,
    VE_FUNC_TERMINATE,
    VE_FUNC_ADD_THREAD,
    VE_FUNC_TERMINATE_THREAD,
    VE_FUNC_MALLOC,
    VE_FUNC_CALLOC,
    VE_FUNC_REALLOC,
    VE_FUNC_MEMALIGN,
    VE_FUNC_FREE,
} ve_func_t;

typedef struct _ve_call_buf
{
    uint64_t func;
    uint64_t retval;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
} ve_call_buf_t;

OE_INLINE void ve_call_buf_clear(volatile ve_call_buf_t* buf)
{
    buf->func = 0;
    buf->retval = 0;
    buf->arg1 = 0;
    buf->arg2 = 0;
    buf->arg3 = 0;
    buf->arg4 = 0;
    buf->arg5 = 0;
    buf->arg6 = 0;
}

typedef struct _ve_init_arg
{
    uint64_t magic;

    /* The enclave end of the socket pair. */
    int sock;

    /* The host shared memory heap. */
    int shmid;
    void* shmaddr;

} ve_init_arg_t;

typedef struct _ve_add_thread_arg
{
    uint64_t tcs;
    uint64_t stack_size;
    int retval;
} ve_add_thread_arg_t;

const char* ve_func_name(ve_func_t func);

int ve_call_send(
    int fd,
    uint64_t func,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6);

OE_INLINE int ve_call_send0(int fd, uint64_t func)
{
    return ve_call_send(fd, func, 0, 0, 0, 0, 0, 0);
}

OE_INLINE int ve_call_send1(int fd, uint64_t func, uint64_t arg1)
{
    return ve_call_send(fd, func, arg1, 0, 0, 0, 0, 0);
}

OE_INLINE int ve_call_send2(int fd, uint64_t func, uint64_t arg1, uint64_t arg2)
{
    return ve_call_send(fd, func, arg1, arg2, 0, 0, 0, 0);
}

OE_INLINE int ve_call_send3(
    int fd,
    uint64_t func,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    return ve_call_send(fd, func, arg1, arg2, arg3, 0, 0, 0);
}

OE_INLINE int ve_call_send4(
    int fd,
    uint64_t func,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4)
{
    return ve_call_send(fd, func, arg1, arg2, arg3, arg4, 0, 0);
}

OE_INLINE int ve_call_send5(
    int fd,
    uint64_t func,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    return ve_call_send(fd, func, arg1, arg2, arg3, arg4, arg5, 0);
}

OE_INLINE int ve_call_send6(
    int fd,
    uint64_t func,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6)
{
    return ve_call_send(fd, func, arg1, arg2, arg3, arg4, arg5, arg6);
}

int ve_call_recv(int fd, uint64_t* retval);

int ve_call(
    int fd,
    uint64_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6);

OE_INLINE int ve_call0(int fd, uint64_t func, uint64_t* retval)
{
    return ve_call(fd, func, retval, 0, 0, 0, 0, 0, 0);
}

OE_INLINE int ve_call1(int fd, uint64_t func, uint64_t* retval, uint64_t arg1)
{
    return ve_call(fd, func, retval, arg1, 0, 0, 0, 0, 0);
}

OE_INLINE int ve_call2(
    int fd,
    uint64_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2)
{
    return ve_call(fd, func, retval, arg1, arg2, 0, 0, 0, 0);
}

OE_INLINE int ve_call3(
    int fd,
    uint64_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    return ve_call(fd, func, retval, arg1, arg2, arg3, 0, 0, 0);
}

OE_INLINE int ve_call4(
    int fd,
    uint64_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4)
{
    return ve_call(fd, func, retval, arg1, arg2, arg3, arg4, 0, 0);
}

OE_INLINE int ve_call5(
    int fd,
    uint64_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5)
{
    return ve_call(fd, func, retval, arg1, arg2, arg3, arg4, arg5, 0);
}

OE_INLINE int ve_call6(
    int fd,
    uint64_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6)
{
    return ve_call(fd, func, retval, arg1, arg2, arg3, arg4, arg5, arg6);
}

#endif /* _VE_CALL_H */
