// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/device.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/errno.h>
#include <openenclave/internal/resolver_ops.h>

static size_t _device_table_len = 0;
static oe_device_t** _device_table = NULL; // Resizable array of device entries

static size_t _fd_table_len = 0;
static oe_device_t** _fd_table = NULL;

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
        _device_table[OE_DEVICE_ENCLAVE_FILESYSTEM], "/", READ_WRITE);

    rslt = (*_device_table[OE_DEVICE_HOST_FILESYSTEM]->ops.fs->mount)(
        _device_table[OE_DEVICE_ENCLAVE_FILESYSTEM], "/host", READ_ONLY);

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

oe_device_t* oe_device_alloc(
    int device_id,
    const char* device_name,
    size_t private_size)

{
    oe_device_t* pparent_device = oe_get_devid_device(device_id);
    oe_device_t* pdevice =
        (oe_device_t*)oe_malloc(pparent_device->size + private_size);
    // We clone the device from the parent device
    oe_memcpy(pdevice, pparent_device, pparent_device->size + private_size);
    pdevice->size = pparent_device->size + private_size;
    pdevice->devicename = device_name; // We do not clone the name

    if (pdevice->ops.base->init != NULL)
    {
        if ((*pdevice->ops.base->init)(pdevice) < 0)
        {
            return NULL;
        }
    }

    return pdevice;
}

int oe_allocate_fd()

{
    // We could increase the size bump to some other number for performance.
    static const size_t SIZE_BUMP = 5;
    static const size_t MIN_FD = 3;

    size_t fd = 0;

    if (_fd_table_len == 0)
    {
        _fd_table_len = SIZE_BUMP;
        _fd_table =
            (oe_device_t**)oe_malloc(sizeof(_fd_table[0]) * _fd_table_len);
        oe_memset(_fd_table, 0, sizeof(_fd_table[0]) * _fd_table_len);

        // Setup stdin, out and error here.

        return MIN_FD; // Leave room for stdin, stdout, and stderr
    }

    for (fd = MIN_FD; fd < _fd_table_len; fd++)
    {
        if (_fd_table[fd] == NULL)
        {
            break;
        }
    }

    if (fd >= _fd_table_len)
    {
        _fd_table_len = (size_t)fd + SIZE_BUMP;
        _fd_table = (oe_device_t**)oe_realloc(
            _fd_table, sizeof(_fd_table[0]) * _fd_table_len);
        _fd_table[fd] = NULL;
    }

    if (_fd_table[fd] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        return -1;
    }

    return (int)fd;
}

void oe_release_fd(int fd)

{
    if (fd < (int)_fd_table_len)
    {
        oe_free(_fd_table[fd]);
        _fd_table[fd] = NULL;
    }
}

oe_device_t* oe_set_fd_device(int fd, oe_device_t* pdevice)

{
    if (fd >= (int)_fd_table_len)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    if (_fd_table[fd] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        return NULL;
    }

    _fd_table[fd] = pdevice; // We don't clone
    return pdevice;
}

oe_device_t* oe_get_fd_device(int fd)

{
    if (fd >= (int)_fd_table_len)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    if (_fd_table[fd] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        return NULL;
    }

    return _fd_table[fd];
}

int oe_clone_fd(int fd)

{
    int newfd = -1;
    oe_device_t* pparent_device = NULL;
    oe_device_t* pnewdevice = NULL;

    if (fd >= (int)_fd_table_len)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    pparent_device = oe_get_devid_device(fd);
    pnewdevice = (oe_device_t*)oe_malloc(pparent_device->size);
    oe_memcpy(pnewdevice, pparent_device, pparent_device->size);

    if (pnewdevice->ops.base->clone != NULL)
    {
        if ((*pnewdevice->ops.base->clone)(pparent_device, &pnewdevice) < 0)
        {
            // Errno is set in the action routine
            return -1;
        }
    }

    newfd = oe_allocate_fd();
    if (oe_set_fd_device(newfd, pnewdevice) == NULL)
    {
        return -1;
    }
    return newfd;
}

int oe_remove_device(int device_id)

{
    int rtn = -1;
    oe_device_t* pdevice = oe_get_devid_device(device_id);

    if (!pdevice)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (pdevice->ops.base->remove != NULL)
    {
        // The action routine sets errno
        rtn = (*pdevice->ops.base->remove)(pdevice);

        if (rtn >= 0)
        {
            oe_release_devid(device_id);
        }
    }
    return rtn;
}

ssize_t oe_read(int fd, void* buf, size_t count)

{
    oe_device_t* pdevice = oe_get_fd_device(fd);
    if (!pdevice)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (pdevice->ops.base->read == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*pdevice->ops.base->read)(pdevice, buf, count);
}

ssize_t oe_write(int fd, const void* buf, size_t count)

{
    oe_device_t* pdevice = oe_get_fd_device(fd);
    if (!pdevice)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (pdevice->ops.base->write == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    return (*pdevice->ops.base->write)(pdevice, buf, count);
}

int oe_close(int fd)

{
    int rtn = -1;
    oe_device_t* pdevice = oe_get_fd_device(fd);

    if (!pdevice)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (pdevice->ops.base->close == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    // The action routine sets errno
    rtn = (*pdevice->ops.base->close)(pdevice);

    if (rtn >= 0)
    {
        oe_release_fd(fd);
    }
    return rtn;
}

int oe_ioctl(int fd, unsigned long request, ...)

{
    oe_va_list ap;
    oe_device_t* pdevice = oe_get_fd_device(fd);
    int rtn = -1;

    if (!pdevice)
    {
        // Log error here
        return -1; // erno is already set
    }

    if (pdevice->ops.base->ioctl == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    oe_va_start(ap, request);
    // The action routine sets errno
    rtn = (*pdevice->ops.base->ioctl)(pdevice, request, ap);
    oe_va_end(ap);

    return rtn;
}
