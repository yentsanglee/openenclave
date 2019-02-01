// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_DEVICE_H
#define _OE_DEVICE_H

#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/epoll_ops.h>
#include <openenclave/internal/errno.h>
#include <openenclave/internal/fs_ops.h>
#include <openenclave/internal/sock_ops.h>

OE_EXTERNC_BEGIN

// Well known  device ids
enum
{
    OE_DEVICE_ID_NONE = 0,

    OE_DEVICE_ID_CONSOLE = 1,

    OE_DEVICE_ID_LOG = 2,

    // This entry describes a file in the hosts's file system
    OE_DEVICE_ID_HOSTFS = 3,

    // The Intel SGX protected file system.
    OE_DEVICE_ID_SGXFS = 4,

    // This entry describes a file in the secure hardward file system.
    OE_DEVICE_ID_SHWFS = 5,

    // This entry describes an internet socket
    OE_DEVICE_ID_HOST_SOCKET = 6,

    // This entry describes an enclave to enclave
    OE_DEVICE_ID_ENCLAVE_SOCKET = 7,

    // This entry describes an epoll instance
    OE_DEVICE_ID_EPOLL = 8,

    // This entry describes an enventfd instance
    OE_DEVICE_ID_EVENTFD = 9
};

typedef enum _oe_device_type
{
    OE_DEVICE_TYPE_NONE = 0,

    OE_DEVICETYPE_FILESYSTEM,

    // This entry describes a file in the hosts's file system
    OE_DEVICETYPE_DIRECTORY,

    // This entry describes an internet socket
    OE_DEVICETYPE_FILE,

    // This entry describes an enclave to enclave
    OE_DEVICETYPE_SOCKET,

    // This entry describes an epoll device
    OE_DEVICETYPE_EPOLL,

    // This entry describes an eventfd device
    OE_DEVICETYPE_EVENTFD
} oe_device_type_t;

// Ready mask. Tracks the values for epoll
static const uint64_t OE_READY_IN = 0x00000001;
static const uint64_t OE_READY_PRI = 0x00000002;
static const uint64_t OE_READY_OUT = 0x00000004;
static const uint64_t OE_READY_ERR = 0x00000008;
static const uint64_t OE_READY_HUP = 0x00000010;
static const uint64_t OE_READY_RDNORM = 0x00000040;
static const uint64_t OE_READY_RDBAND = 0x00000080;
static const uint64_t OE_READY_WRNORM = 0x00000100;
static const uint64_t OE_READY_WRBAND = 0x00000200;
static const uint64_t OE_READY_MSG = 0x00000400;
static const uint64_t OE_READY_RDHUP = 0x00002000;

typedef struct _oe_device
{
    /* Type of this device: OE_DEVICE_ID_FILE or OE_DEVICE_ID_SOCKET. */
    oe_device_type_t type;
    int devid; // Index of the device into the device table.

    /* sizeof additional data. To get a pointer to the device private data,
     * ptr = (oe_file_device_t)(devptr+1); usually sizeof(oe_file_t) or
     * sizeof(oe_socket_t).
     */
    size_t size;
    const char* devicename;

    union {
        oe_device_ops_t* base;
        oe_fs_ops_t* fs;
        oe_sock_ops_t* socket;
        oe_epoll_ops_t* epoll;
    } ops;

} oe_device_t;

int oe_allocate_devid(int devid);
void oe_release_devid(int devid);

int oe_set_devid_device(int device_id, oe_device_t* pdevice);
oe_device_t* oe_get_devid_device(int device_id);
oe_device_t* oe_clone_device(oe_device_t* pdevice);

int oe_device_init(); // Overridable function to set up device structures. Shoud
                      // be ommited when new interface is complete.

int oe_allocate_fd();

void oe_release_fd(int fd);

oe_device_t* oe_device_alloc(
    int device_id,
    const char* device_name,
    size_t private_size); // Allocate a device of sizeof(struct

int oe_device_addref(int device_id);

int oe_device_release(int device_id);

oe_device_t* oe_set_fd_device(int device_id, oe_device_t* pdevice);

oe_device_t* oe_get_fd_device(int fd);

int oe_clone_fd(int fd);

int oe_remove_device();

ssize_t oe_read(int fd, void* buf, size_t count);

ssize_t oe_write(int fd, const void* buf, size_t count);

int oe_close(int fd);

int oe_ioctl(int fd, unsigned long request, ...);

// Take a host fd from hostfs or host_sock and return the enclave file
// descriptor index If the host fd is not found, we return -1
ssize_t oe_map_host_fd(uint64_t host_fd);

typedef struct _oe_device_notification_args
{
    uint64_t host_fd;
    uint64_t notify_mask;
} oe_device_notification_args_t;

oe_result_t _handle_oe_device_notification(uint64_t args);

int oe_handle_device_syscall(
    long num,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long arg5,
    long arg6,
    long* ret_out,
    int* errno_out);

OE_EXTERNC_END

#endif // _OE_DEVICE_H
