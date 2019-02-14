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
#include <openenclave/internal/array.h>

static const size_t ELEMENT_SIZE = sizeof(oe_device_t*);
static const size_t CHUNK_SIZE = 8;
static oe_array_t _arr = OE_ARRAY_INITIALIZER(ELEMENT_SIZE, CHUNK_SIZE);
static oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;
static bool _initialized = false;

OE_INLINE oe_device_t** _table(void)
{
    return (oe_device_t**)_arr.data;
}

OE_INLINE size_t _table_size(void)
{
    return _arr.size;
}

static void _free_table(void)
{
    oe_array_free(&_arr);
}

static int _init_table()
{
    if (_initialized == false)
    {
        oe_spin_lock(&_lock);
        {
            if (_initialized == false)
            {
                if (oe_array_resize(&_arr, CHUNK_SIZE) != 0)
                {
                    oe_assert("_init_table()" == NULL);
                    oe_abort();
                }

                oe_atexit(_free_table);
            }
        }
        oe_spin_unlock(&_lock);
    }

    return 0;
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

    rslt = (*_table()[OE_DEVICE_ID_SGXFS]->ops.fs->mount)(
        _table()[OE_DEVICE_ID_SGXFS], "/", READ_WRITE);

    rslt = (*_table()[OE_DEVICE_ID_HOSTFS]->ops.fs->mount)(
        _table()[OE_DEVICE_ID_SGXFS], "/host", READ_ONLY);

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

oe_devid_t oe_allocate_devid(oe_devid_t devid)
{
    oe_devid_t ret = OE_DEVICE_ID_NONE;
    bool locked = false;

    if (!_initialized && _init_table() != 0)
    {
        oe_errno = OE_ENOMEM;
        goto done;
    }

    oe_spin_lock(&_lock);
    locked = true;

    if (devid >= _arr.size)
    {
        if (oe_array_resize(&_arr, devid + 1) != 0)
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }
    }

    if (_table()[devid] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        goto done;
    }

    ret = devid;

done:

    if (locked)
        oe_spin_unlock(&_lock);

    return ret;
}

int __oe_release_devid(oe_devid_t devid)
{
    int ret = -1;
    bool locked = false;

    if (!_initialized && _init_table() != 0)
    {
        oe_errno = OE_ENOMEM;
        goto done;
    }

    oe_spin_lock(&_lock);
    locked = true;

    if (devid >= _arr.size)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (_table()[devid] == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    _table()[devid] = NULL;

    ret = 0;

done:

    if (locked)
        oe_spin_unlock(&_lock);

    return ret;
}

int oe_set_devid_device(oe_devid_t devid, oe_device_t* device)
{
    int ret = -1;

    if (devid > _table_size())
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (_table()[devid] != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        goto done;
    }

    _table()[devid] = device;

    ret = 0;

done:
    return ret;
}

oe_device_t* oe_get_devid_device(oe_devid_t devid)
{
    oe_device_t* ret = NULL;

    if (devid >= _table_size())
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = _table()[devid];

done:
    return ret;
}

oe_device_t* oe_device_alloc(
    oe_devid_t devid,
    const char* device_name,
    size_t private_size)

{
    oe_device_t* pparent_device = oe_get_devid_device(devid);
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

int oe_remove_device(oe_devid_t devid)
{
    int ret = -1;
    oe_device_t* device;

    if (!(device = oe_get_devid_device(devid)))
        goto done;

    if (device->ops.base->shutdown == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if ((*device->ops.base->shutdown)(device) != 0)
    {
        goto done;
    }

    ret = 0;

done:
    return ret;
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
