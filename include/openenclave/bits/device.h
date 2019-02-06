// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_BITS_DEVICE_H
#define _OE_BITS_DEVICE_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

// Well known  device ids
enum
{
    OE_DEVICE_ID_NONE = 0,

    // This entry describes a file in the hosts's file system
    OE_DEVICE_ID_HOSTFS = 1,

    // The Intel SGX protected file system.
    OE_DEVICE_ID_SGXFS = 2,

    // This entry describes a file in the secure hardward file system.
    OE_DEVICE_ID_SHWFS = 3,

    // This entry describes an internet socket
    OE_DEVICE_ID_HOST_SOCKET = 4,

    // This entry describes an enclave to enclave
    OE_DEVICE_ID_ENCLAVE_SOCKET = 5,

    // This entry describes an epoll instance
    OE_DEVICE_ID_EPOLL = 6,

    // This entry describes an enventfd instance
    OE_DEVICE_ID_EVENTFD = 7
};

OE_EXTERNC_END

#endif // _OE_BITS_DEVICE_H
