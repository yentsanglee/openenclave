// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/array.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/errno.h>
#include <openenclave/internal/fd.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/thread.h>

typedef struct _entry
{
    oe_device_t* device;
} entry_t;

static const size_t ELEMENT_SIZE = sizeof(entry_t);
static const size_t CHUNK_SIZE = 8;
static oe_array_t _arr = OE_ARRAY_INITIALIZER(ELEMENT_SIZE, CHUNK_SIZE);
static oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;
static bool _initialized = false;

OE_INLINE entry_t* _table(void)
{
    return (entry_t*)_arr.data;
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

int oe_assign_fd_device(oe_device_t* device)
{
    int ret = -1;
    size_t index;
    bool locked = false;

    if (_init_table() != 0)
        goto done;

    oe_spin_lock(&_lock);
    locked = true;

    /* Search for a free slot in the file descriptor table. */
    for (index = OE_STDERR_FILENO + 1; index < _table_size(); index++)
    {
        if (!_table()[index].device)
            break;
    }

    /* If free slot not found, expand size of the file descriptor table. */
    if (index == _table_size())
    {
        if (oe_array_resize(&_arr, _table_size() + CHUNK_SIZE) != 0)
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }
    }

    _table()[index].device = device;
    ret = (int)index;

done:

    if (locked)
        oe_spin_unlock(&_lock);

    return ret;
}

void oe_release_fd(int fd)
{
    oe_spin_lock(&_lock);

    if (fd >= 0 && (size_t)fd < _table_size())
    {
        _table()[fd].device = NULL;
    }

    oe_spin_unlock(&_lock);
}

oe_device_t* oe_set_fd_device(int fd, oe_device_t* device)
{
    oe_device_t* ret = NULL;
    bool locked = false;

    if (_init_table() != 0)
        goto done;

    oe_spin_lock(&_lock);
    locked = true;

    if (fd < 0 || (size_t)fd >= _table_size())
    {
        oe_errno = OE_EBADF;
        goto done;
    }

    if (_table()[fd].device != NULL)
    {
        oe_errno = OE_EADDRINUSE;
        goto done;
    }

    _table()[fd].device = device; // We don't clone

    ret = device;

done:

    if (locked)
        oe_spin_unlock(&_lock);

    return ret;
}

oe_device_t* oe_get_fd_device(int fd)
{
    oe_device_t* ret = NULL;
    bool locked = false;

    oe_spin_unlock(&_lock);
    locked = true;

    if (fd < 0 || fd >= (int)_table_size())
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (_table()[fd].device == NULL)
    {
        oe_errno = OE_EBADF;
        goto done;
    }

    ret = _table()[fd].device;

done:

    if (locked)
        oe_spin_unlock(&_lock);

    return ret;
}

#if 0
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
    oe_device_t* device = NULL;

    for (; enclave_fd < (int)_table_size; enclave_fd++)
    {
        device = _table()[enclave_fd];

        if (device != NULL)
        {
            if (device->ops.base->get_host_fd != NULL)
            {
                candidate_host_fd = (*device->ops.base->get_host_fd)(device);

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
#endif
