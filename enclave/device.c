// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "device.h"

static size_t device_table_len = 0;
static oe_device_t* device_table; // Resizable array of device entries

size_t file_descriptor_table_len = 0;
oe_device_t* oe_file_descriptor_table


static size_t resolver_table_len = 0;
static oe_resolver_t* resolver_table; // Resizable array of device entries


void oe_device_init()

{
    // Opt into file systems

    device_table = CreateHostFSDevice(
        device_table, &device_table_len, OE_DEV_HOST_FILESYSTEM);
    OE_DEV_HOST_SOCKET
    OE_DEV_ENCLAVE_SOCKET
    device_table = CreateEnclaveLocalFSDevice(
        device_table, &device_table_len, &EnclaveLocalFSTableIdx);

    int rslt = (*device_table[HostFSTableIdx].mount)("/host", READ_ONLY);
    int rslt =
        (*device_table[EnclaveLocalFSTableIdx].mount)("/secure", READ_WRITE);

    // Opt into the network

    device_table = CreateHostNetInterface(
        device_table, &device_table_len, &HostNetworkTableIdx);
    device_table = CreateEnclaveToEnclaveNetInterface(
        device_table, &device_table_len, &EnclaveLocalNetworkTableIdx);

    // Opt into resolvers

    resolver_table = CreateHostResolver(
        resolver_table, &resolver_table_len, &HostResolverTableIdx);
    resolver_table = CreateEnclaveLocalDNSResolver(
        resolver_table, &resolver_table_len, &EnclaveLocalDNSResolverTableIdx);
    resolver_table = CreateEnclaveLocalResolver(
        resolver_table, &resolver_table_len, &EnclaveLocalResolverTableIdx);
}
