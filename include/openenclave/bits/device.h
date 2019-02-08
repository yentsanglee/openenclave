// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_BITS_DEVICE_H
#define _OE_BITS_DEVICE_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/* This file defines the known device ids. */

#define OE_DEVICE_ID_NONE ((int)0)

#define OE_DEVICE_ID_INSECURE_FS ((int)1)

#define OE_DEVICE_ID_ENCRYPTED_FS ((int)2)

#define OE_DEVICE_ID_SECURE_HARDWARE_FS ((int)3)

/* A host internet socket. */
#define OE_DEVICE_ID_HOST_SOCKET ((int)4)

/* An enclave-to-enclave socket. */
#define OE_DEVICE_ID_ENCLAVE_SOCKET ((int)5)

/* An epoll device. */
#define OE_DEVICE_ID_EPOLL ((int)6)

/* An event file descriptor. */
#define OE_DEVICE_ID_EVENTFD ((int)7)

struct oe_stat;

int oe_device_open(int devid, const char* pathname, int flags, mode_t mode);

OE_DIR* oe_device_opendir(int devid, const char* pathname);

int oe_device_unlink(int devid, const char* pathname);

int oe_device_link(int devid, const char* oldpath, const char* newpath);

int oe_device_rename(int devid, const char* oldpath, const char* newpath);

int oe_device_mkdir(int devid, const char* pathname, mode_t mode);

int oe_device_rmdir(int devid, const char* pathname);

int oe_device_stat(int devid, const char* pathname, struct oe_stat* buf);

int oe_device_truncate(int devid, const char* path, off_t length);

int oe_device_access(int devid, const char* pathname, int mode);

OE_EXTERNC_END

#endif // _OE_BITS_DEVICE_H
