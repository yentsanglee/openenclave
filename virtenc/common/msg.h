// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_MSG_H
#define _VE_MSG_H

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
} ve_func_t;

typedef struct _ve_call_msg
{
    uint64_t func;
    uint64_t arg;
} ve_call_msg_t;

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
    int ret;
} ve_add_thread_arg_t;

const char* ve_func_name(ve_func_t func);

void* ve_malloc(size_t size);

void ve_free(void* ptr);

int ve_recv_n(int fd, void* buf, size_t count);

int ve_send_n(int fd, const void* buf, size_t count);

int ve_call_send(int fd, uint64_t func, uint64_t arg_in);

int ve_call_recv(int fd, uint64_t* arg_out);

int ve_call(int fd, uint64_t func, uint64_t arg_in, uint64_t* arg_out);

#endif /* _VE_MSG_H */
