// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_DEVICE_H
#define _OE_DEVICE_H

#include <openenclave/bits/types.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/errno.h>

OE_EXTERNC_BEGIN

#define OE_PATH_MAX 4096

typedef enum _oe_device_type
{
    OE_DEVICE_NONE,
    OE_DEVICE_FILE,
    OE_DEVICE_SOCKET,
}
oe_device_type_t;

/* Base type for oe_file_t and oe_socket_t. */
typedef struct _oe_device
{
    /* Type of this device: OE_DEVICE_FILE or OE_DEVICE_SOCKET. */
    oe_device_type_t type;

    /* Size of full structure including extended size. */
    size_t size;

    ssize_t (*read)(int fd, void *buf, size_t count);

    ssize_t (*write)(int fd, const void *buf, size_t count);

    int (*ioctl)(int fd, unsigned long request, ...);

    int (*close)(int fd);
}
oe_device_t;

typedef struct _oe_device_entry
{
    oe_device_type_t type;
    char* devicename;
    uint64_t flags;
    oe_device_t* device;
}
oe_device_entry_t;

int oe_assign_fd(void);

int oe_release_fd(int fd);

oe_device_t* oe_fd_to_device(int fd);

extern oe_device_entry_t* oe_file_descriptor_table;
extern size_t oe_file_descriptor_table_len;

OE_EXTERNC_END

#endif // _OE_DEVICE_H
