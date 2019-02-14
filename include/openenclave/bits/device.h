// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_BITS_DEVICE_H
#define _OE_BITS_DEVICE_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

typedef uint64_t oe_devid_t;

/* This is an illegal device identifier. */
static const oe_devid_t OE_DEVID_NONE = 0;

/* The nonsecure host file system. */
static const oe_devid_t OE_DEVID_HOSTFS = 1;

/* The secure Intel SGX protected file system. */
static const oe_devid_t OE_DEVID_SGXFS = 2;

/* The ARM TrustZone secure hardware file system. */
static const oe_devid_t OE_DEVID_SHWFS = 3;

/* A host internet socket. */
static const oe_devid_t OE_DEVID_HOST_SOCKET = 4;

/* An enclave-to-enclave socket. */
static const oe_devid_t OE_DEVID_ENCLAVE_SOCKET = 5;

/* An epoll device. */
static const oe_devid_t OE_DEVID_EPOLL = 6;

/* An event file descriptor. */
static const oe_devid_t OE_DEVID_EVENTFD = 7;

struct oe_stat;
typedef struct _OE_FILE OE_FILE;

OE_EXTERNC_END

#endif // _OE_BITS_DEVICE_H
