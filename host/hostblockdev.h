// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _oe_host_blockdevice_h
#define _oe_host_blockdevice_h

#include <openenclave/host.h>

void oe_handle_open_blkdev(
    oe_enclave_t* enclave,
    uint64_t arg_in,
    uint64_t* arg_out);

void oe_handle_close_blkdev(oe_enclave_t* enclave, uint64_t arg_in);

void oe_handle_blkdev_get(oe_enclave_t* enclave, uint64_t arg_in);

void oe_handle_blkdev_put(oe_enclave_t* enclave, uint64_t arg_in);

#endif /* _oe_host_blockdevice_h */
