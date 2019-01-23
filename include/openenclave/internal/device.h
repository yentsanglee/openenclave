// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_DEVICE_H
#define _OE_DEVICE_H

#include <openenclave/bits/types.h>
#include <openenclave/internal/errno.h>
#include <openenclave/internal/fs_ops.h>
#include <openenclave/internal/resolver_ops.h>
#include <openenclave/internal/sock_ops.h>

OE_EXTERNC_BEGIN

typedef enum _oe_device_type
{
    OE_DEVICE_NONE = 0,           // This entry is invalid
    OE_DEVICE_CONSOLE,            // Console device for stdin and stdout
    OE_DEVICE_LOG,                // log device for stderr
    OE_DEVICE_ENCLAVE_FILESYSTEM, // This entry describes a file in the enclaves
                                  // secure file system
    OE_DEVICE_HOST_FILESYSTEM,    // This entry describes a file in the hosts's
                               // file system
    OE_DEVICE_HOST_SOCKET,   // This entry describes an internet socket
    OE_DEVICE_ENCLAVE_SOCKET // This entry describes an enclave to enclave
} oe_device_type_t;

typedef struct _oe_device
{
    /* Type of this device: OE_DEVICE_FILE or OE_DEVICE_SOCKET. */
    oe_device_type_t type;

    /* sizeof additional data. To get a pointer to the device private data,
     * ptr = (oe_file_device_t)(devptr+1); usually sizeof(oe_file_t) or
     * sizeof(oe_socket_t).
     */
    size_t size;
    const char* devicename;

    union
    {
        oe_device_ops_t* base;
        oe_fs_ops_t* fs;
        oe_sock_ops_t* socket;
    }
    ops;

} *oe_device_t;

typedef struct _oe_device_entry
{
    oe_device_type_t type;
    uint64_t flags;
    oe_device_t device;
} oe_device_entry_t;

int oe_allocate_devid(int devid);
void oe_release_devid(int devid);

oe_device_t oe_set_devid_device(int device_id, oe_device_t pdevice);
oe_device_t oe_get_devid_device(int fd);

int oe_device_init(); // Overridable function to set up device structures. Shoud
                      // be ommited when new interface is complete.

int oe_allocate_fd();

void oe_release_fd(int fd);

oe_device_t oe_device_alloc(
    int device_id,
    const char* device_name,
    size_t private_size); // Allocate a device of sizeof(struct

int oe_device_addref(int device_id);

int oe_device_release(int device_id);

oe_device_t oe_set_fd_device(int device_id, oe_device_t pdevice);

oe_device_t oe_get_fd_device(int fd);

int oe_clone_fd(int fd);

int oe_remove_device();

ssize_t oe_read(int fd, void* buf, size_t count);

ssize_t oe_write(int fd, const void* buf, size_t count);

int oe_close(int fd);

int oe_ioctl(int fd, unsigned long request, ...);

OE_EXTERNC_END

#endif // _OE_DEVICE_H
