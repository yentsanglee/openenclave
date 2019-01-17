// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
**==============================================================================
**
** host_socket.h
**
**     Definition of the host_socket internal types and data
**
**==============================================================================
*/

#ifndef _OE_HOST_SOCKET_H__
#define _OE_HOST_SOCKET_H__

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/defs.h>
#include <openenclave/internal/device.h>

OE_EXTERNC_BEGIN

oe_device_t* CreateHostSocketDevice(
    iconst oe_device_t* device_table,
    int32_t* pdevice_table_len,
    int device_table_index);

OE_EXTERNC_END

#endif /* _OE_EPID_H */
