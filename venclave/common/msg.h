// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_MSG_H
#define _VE_MSG_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/sgxtypes.h>

#define VE_MSG_MAGIC 0x2e40afd8dee443f8

typedef enum _ve_msg_type
{
    VE_MSG_TERMINATE,
    VE_MSG_EGETKEY,
} ve_msg_type_t;

typedef struct _ve_msg
{
    uint64_t magic;
    uint64_t type;
    uint64_t size;
    uint8_t data[];
} ve_msg_t;

typedef struct _ve_egetkey_request
{
    sgx_key_request_t request;
} ve_egetkey_request_t;

typedef struct _ve_egetkey_response
{
    uint64_t ret;
    sgx_key_t key;
} ve_egetkey_response_t;

int ve_msg_send(int fd, ve_msg_type_t type, const void* data, size_t size);

int ve_msg_recv(int fd, ve_msg_type_t type, void* data, size_t size);

int ve_msg_recv_any(int fd, ve_msg_type_t* type, void** data, size_t* size);

#endif /* _VE_MSG_H */
