// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_BITS_DEVICE_H
#define _OE_BITS_DEVICE_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/* This file defines the known device ids. */

#define OE_DEVID_NONE ((int)0)

#define OE_DEVID_INSECURE_FILE_SYSTEM ((int)1)

#define OE_DEVID_ENCRYPTED_FILE_SYSTEM ((int)2)

#define OE_DEVID_SECURE_HARDWARE_FILE_SYSTEM ((int)3)

/* A host internet socket. */
#define OE_DEVID_HOST_SOCKET ((int)4)

/* An enclave-to-enclave socket. */
#define OE_DEVID_ENCLAVE_SOCKET ((int)5)

/* An epoll device. */
#define OE_DEVID_EPOLL ((int)6)

/* An event file descriptor. */
#define OE_DEVID_EVENTFD ((int)7)

OE_EXTERNC_END

#endif // _OE_BITS_DEVICE_H
