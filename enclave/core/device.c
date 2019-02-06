// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/device.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/errno.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/thread.h>

static size_t _device_table_len = 0;
static oe_device_t** _device_table = NULL; // Resizable array of device entries

static void _free_device_table(void)
{
    oe_free(_device_table);
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

    if (CreateHostFSDevice(OE_DEVICE_ID_INSECURE_FS) < 0)
    {
        // Log an error and continue
    }

    if (CreateEnclaveLocalFSDevice(OE_DEVICE_ID_ENCRYPTED_FS) < 0)
    {
        // Log an error and continue
    }

    rslt = (*_device_table[OE_DEVICE_ID_ENCRYPTED_FS]->ops.fs->mount)(
        _device_table[OE_DEVICE_ID_ENCRYPTED_FS], "/", READ_WRITE);

    rslt = (*_device_table[OE_DEVICE_ID_INSECURE_FS]->ops.fs->mount)(
        _device_table[OE_DEVICE_ID_ENCRYPTED_FS], "/host", READ_ONLY);

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
    ssize_t ret = -1;
    oe_device_t* device;
    ssize_t n;

    if (!(device = oe_get_fd_device(fd)))
        goto done;

    if (device->ops.base->read == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    // The action routine sets errno
    if ((n = (*device->ops.base->read)(device, buf, count)) < 0)
        goto done;

    ret = n;

done:
    return ret;
}

ssize_t oe_write(int fd, const void* buf, size_t count)
{
    ssize_t ret = -1;
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
    int ret = -1;

    switch (fd)
    {
        case OE_STDIN_FILENO:
        case OE_STDERR_FILENO:
        case OE_STDOUT_FILENO:
        {
            static const unsigned long _TIOCGWINSZ = 0x5413;

            if (request == _TIOCGWINSZ)
            {
                struct winsize
                {
                    unsigned short int ws_row;
                    unsigned short int ws_col;
                    unsigned short int ws_xpixel;
                    unsigned short int ws_ypixel;
                };
                oe_va_list ap;
                struct winsize* p;

                oe_va_start(ap, request);
                p = oe_va_arg(ap, struct winsize*);
                oe_va_end(ap);

                if (!p)
                    goto done;

                p->ws_row = 24;
                p->ws_col = 80;
                p->ws_xpixel = 0;
                p->ws_ypixel = 0;

                ret = 0;
                goto done;
            }

            ret = -1;
            goto done;
        }
        default:
        {
            oe_va_list ap;
            oe_device_t* pdevice = oe_get_fd_device(fd);

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
            ret = (*pdevice->ops.base->ioctl)(pdevice, request, ap);
            oe_va_end(ap);
            goto done;
        }
    }

done:
    return ret;
}
