// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_BITS_DEVICE_H
#define _OE_BITS_DEVICE_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/* This file defines the known device ids. */

typedef int64_t oe_devid_t;

#define OE_DEVICE_ID_NONE ((oe_devid_t)0)

#define OE_DEVICE_ID_HOSTFS ((oe_devid_t)1)

#define OE_DEVICE_ID_SGXFS ((oe_devid_t)2)

#define OE_DEVICE_ID_SHWFS ((oe_devid_t)3)

/* A host internet socket. */
#define OE_DEVICE_ID_HOST_SOCKET ((oe_devid_t)4)

/* An enclave-to-enclave socket. */
#define OE_DEVICE_ID_ENCLAVE_SOCKET ((oe_devid_t)5)

/* An epoll device. */
#define OE_DEVICE_ID_EPOLL ((oe_devid_t)6)

/* An event file descriptor. */
#define OE_DEVICE_ID_EVENTFD ((oe_devid_t)7)

struct oe_stat;
typedef struct _OE_FILE OE_FILE;

OE_EXTERNC_END

#endif // _OE_BITS_DEVICE_H
