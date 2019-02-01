// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/device.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/errno.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/print.h>

static size_t _device_table_len = 0;
static oe_device_t** _device_table = NULL; // Resizable array of device entries

static size_t _fd_table_len = 0;
static oe_device_t** _fd_table = NULL;

static void _free_device_table(void)
{
    oe_free(_device_table);
}

static void _free_fd_table(void)
{
    oe_free(_fd_table);
}

// We define the device init for now. Eventually it should be a mandatory part
// of the enclave
//#define DEFAULT_DEVICE_INIT
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

    if (CreateHostFSDevice(OE_DEVICE_ID_HOSTFS) < 0)
    {
        // Log an error and continue
    }

    if (CreateEnclaveLocalFSDevice(OE_DEVICE_ID_SGXFS) < 0)
    {
        // Log an error and continue
    }

    rslt = (*_device_table[OE_DEVICE_ID_SGXFS]->ops.fs->mount)(
        _device_table[OE_DEVICE_ID_SGXFS], "/", READ_WRITE);

    rslt = (*_device_table[OE_DEVICE_ID_HOSTFS]->ops.fs->mount)(
        _device_table[OE_DEVICE_ID_SGXFS], "/host", READ_ONLY);

    // Opt into the network

    if (CreateHostNetInterface(OE_DEVICE_ID_HOST_SOCKET) < 0)
    {
        // Log an error and continue
    }

    if (CreateEnclaveToEnclaveNetInterface(OE_DEVICE_ID_ENCLAVE_SOCKET) < 0)
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
        oe_atexit(_free_device_table);
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

int oe_set_devid_device(int devid, oe_device_t* pdevice)
{
    int ret = -1;

    if (devid < 0 || (size_t)devid > _device_table_len)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (_device_table[devid] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        goto done;
    }

    _device_table[devid] = pdevice; // We don't clone

    ret = 0;

done:
    return ret;
}

oe_device_t* oe_get_devid_device(int devid)
{
    if (devid < 0 || (size_t)devid >= _device_table_len)
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

#if 0
    if (pdevice->ops.base->init != NULL)
    {
        if ((*pdevice->ops.base->init)(pdevice) < 0)
        {
            return NULL;
        }
    }
#endif

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
        _fd_table = oe_calloc(1, sizeof(_fd_table[0]) * _fd_table_len);
        oe_atexit(_free_fd_table);

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
    if (fd >= 0 && (size_t)fd < _fd_table_len)
    {
        _fd_table[fd] = NULL;
    }
}

oe_device_t* oe_set_fd_device(int fd, oe_device_t* device)
{
    oe_device_t* ret = NULL;

    if (fd < 0 || (size_t)fd >= _fd_table_len)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (_fd_table[fd] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        goto done;
    }

    _fd_table[fd] = device; // We don't clone

    ret = device;

done:
    return ret;
}

oe_device_t* oe_get_fd_device(int fd)
{
    if (fd >= (int)_fd_table_len)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    if (_fd_table[fd] == NULL)
    {
        oe_errno = OE_EBADF;
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

    pparent_device = oe_get_fd_device(fd);
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

    if (pdevice->ops.base->shutdown != NULL)
    {
        // The action routine sets errno
        rtn = (*pdevice->ops.base->shutdown)(pdevice);

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
    ssize_t ret = -1;

    if (fd == OE_STDOUT_FILENO || fd == OE_STDERR_FILENO)
    {
        if (oe_host_write(fd - 1, (const char*)buf, count) != 0)
        {
            oe_errno = OE_EIO;
            goto done;
        }

        ret = (ssize_t)count;
    }
    else
    {
        oe_device_t* device;

        if (!(device = oe_get_fd_device(fd)))
        {
            oe_errno = OE_EBADF;
            goto done;
        }

        if (device->ops.base->write == NULL)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        // The action routine sets errno
        ret = (*device->ops.base->write)(device, buf, count);
    }

done:
    return ret;
}

int oe_close(int fd)
{
    int ret = -1;
    oe_device_t* device = oe_get_fd_device(fd);

    if (!device)
    {
        goto done;
    }

    if (device->ops.base->close == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    if ((*device->ops.base->close)(device) != 0)
    {
        goto done;
    }

    oe_release_fd(fd);

    ret = 0;

done:
    return ret;
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

oe_result_t _handle_oe_device_notification(uint64_t arg)
{
    oe_device_notification_args_t* pargs = (oe_device_notification_args_t*)arg;
    uint64_t host_fd = pargs->host_fd;
    uint64_t notification_mask = pargs->notify_mask;
    int enclave_fd = 0;
    oe_device_t* pdevice = NULL;
    int rtn = -1;

    enclave_fd = (int)oe_map_host_fd(host_fd);
    if (enclave_fd >= 0)
    {
        pdevice = oe_get_fd_device(enclave_fd);
        rtn = (*pdevice->ops.base->notify)(pdevice, notification_mask);
    }

    if (rtn != -1)
    {
        return OE_OK;
    }
    else
    {
        return OE_FAILURE;
    }
}

// This is a very naive implementation of association between hostfd and enclave
// fd Since it does a linear search of the fd table it could become a
// performance issue, but that hasn't happened yet and is unlikely to do so
// immediately. If per does become an issue, the host_fd could be hashed to an 8
// bit hash value by treating it as a sum of 8 bytes then look for the value in
// one of 256 buckets.

ssize_t oe_map_host_fd(uint64_t host_fd)

{
    const static ssize_t NOT_FOUND = -1;
    int enclave_fd = 0;
    ssize_t candidate_host_fd = 0;
    oe_device_t* pdevice = NULL;

    for (; enclave_fd < (int)_fd_table_len; enclave_fd++)
    {
        pdevice = _fd_table[enclave_fd];
        if (pdevice != NULL)
        {
            if (pdevice->ops.base->get_host_fd != NULL)
            {
                candidate_host_fd = (*pdevice->ops.base->get_host_fd)(pdevice);
                // host_fd cannot be -1. So if we return -1 the comparison will
                // always fail
                if (candidate_host_fd == (ssize_t)host_fd)
                {
                    return enclave_fd;
                }
            }
        }
    }
    return NOT_FOUND;
}
