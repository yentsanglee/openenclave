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

#define VE_SHMADDR_MAGIC 0xc500f4b6cd2c4f42

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

typedef struct _ve_msg_ping_thread_in
{
    int value;
    char* str;
} ve_msg_ping_thread_in_t;

typedef struct _ve_msg_ping_thread_out
{
    int value;
    int ret;
} ve_msg_ping_thread_out_t;

typedef struct _ve_msg_init_in
{
    /* Child's end of the socket pair that the parent created. */
    int sock;

    /* Host's shared memory heap. */
    int shmid;
    void* shmaddr;
} ve_msg_init_in_t;

typedef struct _ve_msg_init_out
{
    int ret;
} ve_msg_init_out_t;

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

void ve_msg_free(ve_msg_t* msg);

int ve_recv_n(int fd, void* buf, size_t count);

int ve_send_n(int fd, const void* buf, size_t count);

int ve_send_msg(int fd, ve_msg_type_t type, const void* data, size_t size);

int ve_recv_msg(int fd, ve_msg_t* msg);

int ve_recv_msg_by_type(int fd, ve_msg_type_t type, void* data, size_t size);

typedef enum _ve_func
{
    VE_FUNC_RET,
    VE_FUNC_ERR,
    VE_FUNC_PING,
} ve_func_t;

typedef struct _ve_call_msg
{
    uint64_t func;
    uint64_t arg;
} ve_call_msg_t;

int ve_call(int fd, uint64_t func, uint64_t arg_in, uint64_t* arg_out);

#endif /* _VE_MSG_H */
