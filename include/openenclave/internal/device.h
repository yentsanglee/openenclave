// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_DEVICE_H
#define _OE_DEVICE_H

#include <openenclave/bits/types.h>
#include <openenclave/internal/errno.h>
#include <openenclave/internal/fs_ops.h>
#include <openenclave/internal/sock_ops.h>

OE_EXTERNC_BEGIN

typedef enum _oe_device_type
{
    OE_DEV_NONE = 0,      // This entry is invalid
    OE_DEV_SECURE_FILE,   // This entry describes a file in the enclaves
                          // secure file system
    OE_DEV_HOST_FILE,     // This entry describes a file in the hosts's file
                          // system
    OE_DEV_SOCKET,        // This entry describes an internet socket
    OE_DEV_ENCLAVE_SOCKET // This entry describes an enclave to enclave
} oe_device_type_t;

/* Base type for oe_file_t and oe_socket_t. */
typedef struct _oe_device
{
    /* Type of this device: OE_DEVICE_FILE or OE_DEVICE_SOCKET. */
    oe_device_type_t type;

    /* sizeof additional data. To get a pointer to the device private data,
     * ptr = (oe_file_device_t)(devptr+1); usually sizeof(oe_file_t) or
     * sizeof(oe_socket_t).
     */
    size_t size;

    union
    {
        oe_fs_ops_t fs;
        oe_sock_ops_t socket;
    }
    ops;

} oe_device_t;

typedef struct _oe_device_entry
{
    oe_device_type_t type;
    char* devicename;
    uint64_t flags;
    oe_device_t* device;
} oe_device_entry_t;

int oe_device_init(); // Overridable function to set up device structures. Shoud
                      // be ommited when new interface is complete.

int oe_allocate_fd();

void oe_release_fd(int fd);

oe_device_t* oe_device_alloc(
    int device_id,
    char* device_name,
    int private_size); // Allocate a device of sizeof(struct

int oe_device_addref(int device_id);

int oe_device_release(int device_id);

oe_device_t* oe_set_fd_device(int device_id, oe_device_t* pdevice);

oe_device_t* oe_get_fd_device(int fd);

oe_device_t* oe_fd_to_device(int fd);

OE_EXTERNC_END

#endif // _OE_DEVICE_H
