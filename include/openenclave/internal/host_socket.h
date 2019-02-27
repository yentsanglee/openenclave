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

#include <openenclave/internal/device.h>

OE_EXTERNC_BEGIN

oe_device_t* oe_socket_get_hostsock();
void oe_socket_install_hostsock();

OE_EXTERNC_END

#endif /* _OE_EPID_H */
