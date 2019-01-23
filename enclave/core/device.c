// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/device.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/errno.h>
#include <openenclave/internal/resolver_ops.h>

static size_t _device_table_len = 0;
static oe_device_t** _device_table = NULL; // Resizable array of device entries

#if 0 // ATTN: suppress unused error.
static size_t _fd_table_len = 0;
static oe_device_t** _oe_fd_table = NULL;
#endif

#if 0
static size_t resolver_table_len = 0;
static oe_resolver_t** resolver_table =
    NULL; // Resizable array of device entries
#endif

// We define the device init for now. Eventually it should be a mandatory part
// of the enclave
#define DEFAULT_DEVICE_INIT
#if defined(DEFAULT_DEVICE_INIT)

extern int CreateHostFSDevice(oe_device_type_t device_id);
extern int CreateEnclaveLocalFSDevice(oe_device_type_t device_id);

extern int CreateHostNetInterface(oe_device_type_t device_id);
extern int CreateEnclaveToEnclaveNetInterface(oe_device_type_t device_id);

extern int CreateHostResolver();
extern int CreateEnclaveLocalDNSResolver();
extern int CreateEnclaveLocalResolver();

#define READ_WRITE 1
#define READ_ONLY 2

int oe_device_init()

{
    int rslt = -1;

    // Opt into file systems

    if (CreateHostFSDevice(OE_DEVICE_HOST_FILESYSTEM) < 0)
    {
        // Log an error and continue
    }

    if (CreateEnclaveLocalFSDevice(OE_DEVICE_ENCLAVE_FILESYSTEM) < 0)
    {
        // Log an error and continue
    }

    rslt = (*_device_table[OE_DEVICE_ENCLAVE_FILESYSTEM]->ops.fs->mount)(
        _device_table[OE_DEVICE_ENCLAVE_FILESYSTEM],
        "/", READ_WRITE);

    rslt = (*_device_table[OE_DEVICE_HOST_FILESYSTEM]->ops.fs->mount)(
        _device_table[OE_DEVICE_ENCLAVE_FILESYSTEM],
        "/host", READ_ONLY);

    // Opt into the network

    if (CreateHostNetInterface(OE_DEVICE_HOST_SOCKET) < 0)
    {
        // Log an error and continue
    }

    if (CreateEnclaveToEnclaveNetInterface(OE_DEVICE_ENCLAVE_SOCKET) < 0)
    {
        // Log an error and continue
    }

    // Opt into resolvers

    if (CreateHostResolver() < 0)
    {
        // Log an error and continue
    }

    if (CreateEnclaveLocalDNSResolver() < 0)
    {
        // Log an error and continue
    }

    if (CreateEnclaveLocalResolver() < 0)
    {
        // Log an error and continue
    }

    return rslt;
}

#endif

int oe_allocate_devid(int devid)

{
    // We could increase the size bump to some other number for performance.
    static const int SIZE_BUMP = 5;

    if (_device_table_len == 0)
    {
        _device_table_len = (size_t)devid + SIZE_BUMP;
        _device_table = (oe_device_t**)oe_malloc(
            sizeof(_device_table[0]) * _device_table_len);
        oe_memset(
            _device_table, 0, sizeof(_device_table[0]) * _device_table_len);
    }
    else if (devid >= (int)_device_table_len)
    {
        _device_table_len = (size_t)devid + SIZE_BUMP;
        _device_table = (oe_device_t**)oe_realloc(
            _device_table, sizeof(_device_table[0]) * _device_table_len);
        _device_table[devid] = NULL;
    }

    if (_device_table[devid] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        return -1;
    }

    return devid;
}

void oe_release_devid(int devid)

{
    if (devid < (int)_device_table_len)
    {
        oe_free(_device_table[devid]);
        _device_table[devid] = NULL;
    }
}

oe_device_t* oe_set_devid_device(int device_id, oe_device_t* pdevice)

{
    if (device_id >= (int)_device_table_len)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    if (_device_table[device_id] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        return NULL;
    }

    _device_table[device_id] = pdevice; // We don't clone
    return pdevice;
}

oe_device_t* oe_get_devid_device(int devid)

{
    if (devid >= (int)_device_table_len)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    return _device_table[devid];
}

#if 0 // ATTN: does not compile yet.
oe_device_t* oe_device_alloc(
    int device_id,
    const char* device_name,
    int private_size)

{
    oe_device_t* pparent_device = oe_get_devid_device(device_id);
    oe_device_t* pdevice = (oe_device_t*)oe_malloc(
        pparent_device->size + private_size); // Private size is
    oe_memcpy(
        pdevice, pparent_device); // We clone the device from the parent device
    pdevice->device_name = device_name; // We do not clone the name

    return pdevice;
}
#endif

int oe_allocate_fd();

void oe_release_fd(int fd);

oe_device_t* oe_set_fd_device(int device_id, oe_device_t* pdevice);

oe_device_t* oe_get_fd_device(int fd);
