// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_BITS_DEVICE_H
#define _OE_BITS_DEVICE_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/* This file defines the known device ids. */

#define OE_DEVICE_ID_NONE ((int)0)

#define OE_DEVICE_ID_HOSTFS ((int)1)

#define OE_DEVICE_ID_SGXFS ((int)2)

#define OE_DEVICE_ID_SHWFS ((int)3)

/* A host internet socket. */
#define OE_DEVICE_ID_HOST_SOCKET ((int)4)

/* An enclave-to-enclave socket. */
#define OE_DEVICE_ID_ENCLAVE_SOCKET ((int)5)

/* An epoll device. */
#define OE_DEVICE_ID_EPOLL ((int)6)

/* An event file descriptor. */
#define OE_DEVICE_ID_EVENTFD ((int)7)

OE_EXTERNC_END

#endif // _OE_BITS_DEVICE_H
