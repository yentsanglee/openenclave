// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_CALL_H
#define _VE_CALL_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/corelibc/limits.h>

#define VE_INIT_ARG_MAGIC 0x3b146763853147ce

typedef enum _ve_func
{
    VE_FUNC_RET,
    VE_FUNC_ERR,
    VE_FUNC_INIT,
    VE_FUNC_POST_INIT,
    VE_FUNC_TERMINATE,
    VE_FUNC_ADD_THREAD,
    VE_FUNC_TERMINATE_THREAD,
    VE_FUNC_MALLOC,
    VE_FUNC_CALLOC,
    VE_FUNC_REALLOC,
    VE_FUNC_MEMALIGN,
    VE_FUNC_FREE,
    VE_FUNC_GET_SETTINGS,
    VE_FUNC_INIT_ENCLAVE,
    VE_FUNC_CALL_ENCLAVE_FUNCTION,
    VE_FUNC_CALL_HOST_FUNCTION,
    VE_FUNC_OCALL,
    VE_FUNC_ABORT,
} ve_func_t;

typedef struct _ve_call_buf
{
    /* The process id of the sender. */
    uint64_t func;
    uint64_t retval;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
} ve_call_buf_t;

typedef struct _ve_enclave_settings
{
    uint64_t num_heap_pages;
    uint64_t num_stack_pages;
    uint64_t num_tcs;
} ve_enclave_settings_t;

/* Prevent clang from replacing this with crashing struct initialization. */
OE_INLINE void ve_call_buf_clear(volatile ve_call_buf_t* buf)
{
    if (buf)
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
}

typedef struct _ve_init_arg
{
    uint64_t magic;
    int sock;
    int shmid;
    void* shmaddr;
    size_t shmsize;
    size_t tdata_rva;
    size_t tdata_size;
    size_t tdata_align;
    size_t tbss_rva;
    size_t tbss_size;
    size_t tbss_align;
    uint64_t self_rva;
    uint64_t base_rva;
    char vproxyhost_path[OE_PATH_MAX];
} ve_init_arg_t;

const char* ve_func_name(ve_func_t func);

int ve_call_send(
    int fd,
    ve_func_t func,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6);

OE_INLINE int ve_call_send0(int fd, ve_func_t func)
{
    return ve_call_send(fd, func, 0, 0, 0, 0, 0, 0);
}

OE_INLINE int ve_call_send1(int fd, ve_func_t func, uint64_t arg1)
{
    return ve_call_send(fd, func, arg1, 0, 0, 0, 0, 0);
}

OE_INLINE int ve_call_send2(
    int fd,
    ve_func_t func,
    uint64_t arg1,
    uint64_t arg2)
{
    return ve_call_send(fd, func, arg1, arg2, 0, 0, 0, 0);
}

OE_INLINE int ve_call_send3(
    int fd,
    ve_func_t func,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    return ve_call_send(fd, func, arg1, arg2, arg3, 0, 0, 0);
}

OE_INLINE int ve_call_send4(
    int fd,
    ve_func_t func,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4)
{
    return ve_call_send(fd, func, arg1, arg2, arg3, arg4, 0, 0);
}

OE_INLINE int ve_call_send5(
    int fd,
    ve_func_t func,
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
    ve_func_t func,
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
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6);

OE_INLINE int ve_call0(int fd, ve_func_t func, uint64_t* retval)
{
    return ve_call(fd, func, retval, 0, 0, 0, 0, 0, 0);
}

OE_INLINE int ve_call1(int fd, ve_func_t func, uint64_t* retval, uint64_t arg1)
{
    return ve_call(fd, func, retval, arg1, 0, 0, 0, 0, 0);
}

OE_INLINE int ve_call2(
    int fd,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2)
{
    return ve_call(fd, func, retval, arg1, arg2, 0, 0, 0, 0);
}

OE_INLINE int ve_call3(
    int fd,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3)
{
    return ve_call(fd, func, retval, arg1, arg2, arg3, 0, 0, 0);
}

OE_INLINE int ve_call4(
    int fd,
    ve_func_t func,
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
    ve_func_t func,
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
    ve_func_t func,
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
