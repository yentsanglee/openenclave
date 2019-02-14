// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_INTERNAL_FS_H
#define _OE_INTERNAL_FS_H

#include <openenclave/bits/fs.h>
#include <openenclave/internal/device.h>

OE_EXTERNC_BEGIN

/* Get the access mode from the open() flags. */
OE_INLINE int oe_get_open_access_mode(int flags)
{
    return (flags & 000000003);
}

/* The enclave calls this to get an instance of host file system (HOSTFS). */
oe_device_t* oe_fs_get_hostfs(void);

/* The host calls this to install the host file system (SGXFS). */
void oe_fs_install_sgxfs(void);

/* The enclave calls this to get an instance of host file system (SGXFS). */
oe_device_t* oe_fs_get_sgxfs(void);

int oe_register_hostfs_device(void);

int oe_register_sgxfs_device(void);

/* Initialize the stdin, stdout, and stderr devices. */
int oe_initialize_console_devices(void);

/* Set the default device for this thread (used in lieu of the mount table). */
int oe_set_thread_default_device(int devid);

/* Clear the default device for this thread. */
int oe_clear_thread_default_device(void);

/* Get the default device for this thread. */
int oe_get_thread_default_device(void);

OE_EXTERNC_END

#endif // _OE_INTERNAL_FS_H
