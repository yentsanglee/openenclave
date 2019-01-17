/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */
#include "host_socket.h"
#include <openenclave/enclave.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/errno.h>

struct oe_host_socket_desc
{
};

static

    // Populates Returning device_table allows us to realloc if needed.

    oe_device_t*
    CreateHostSocketDevice(
        oe_device_t* device_table,
        int32_t* pdevice_table_len,
        int device_table_index)

{
    oe_device_t* new_device_table = oe_device_alloc(device_table_index,  "host_sockets", sizeof(struct oe_device_entry)+sizeof((oe_host_socket_desc));
    oe_device_t* pentry = NULL;

    if (new_device_table == NULL)
    {
        return new_device_table;
    }


    return new_device_table;
}
