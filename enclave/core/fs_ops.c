// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/device.h>
#include <openenclave/internal/enclavelibc.h>

typedef struct _mount_point
{
    size_t pathlen; // an optimisation. I can easily skip paths that are the
                    // wrong length without needing to do a string compare.
    const char* mount_path;
    oe_device_t* fs;
    uint32_t flags;
} oe_mount_point_t;

const size_t _MOUNT_TABLE_SIZE_BUMP = 5;

size_t _mount_table_len = 0;
oe_mount_point_t* _mount_table = NULL;

static oe_device_t* _fs_lookup(const char* path, char suffix[OE_PATH_MAX])
{
    oe_device_t* ret = NULL;
    size_t match_len = 0;

    if (!path)
        goto done;

    // pthread_spin_lock(&_lock);
    {
        /* Find the longest binding point that contains this path. */
        for (size_t i = 0; i < _mount_table_len; i++)
        {
            size_t len = _mount_table[i].pathlen;

            if (oe_strncmp(_mount_table[i].mount_path, path, len) == 0 &&
                (path[len] == '/' || path[len] == '\0'))
            {
                if (len > match_len)
                {
                    if (suffix)
                    {
                        oe_strlcpy(suffix, path + len, OE_PATH_MAX);
                    }

                    match_len = len;
                    ret = _mount_table[i].fs;
                }
            }
        }
    }
    // pthread_spin_unlock(&_lock);

done:

    return ret;
}

int oe_mount(int device_id, const char* path, uint32_t flags)

{
    size_t mount_path_len = (size_t)-1;
    size_t new_element = 0;
    oe_device_t* pfs_device = oe_get_devid_device(device_id);
    int new_devid = -1;

    (void)mount_path_len;
    (void)new_devid;
    (void)flags;

    if (pfs_device == NULL)
    {
        oe_errno = OE_EBADFD;
        return -1;
    }

    if (path == NULL)
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (_mount_table == NULL)
    {
        _mount_table_len = _MOUNT_TABLE_SIZE_BUMP;
        _mount_table = (oe_mount_point_t*)oe_calloc(1,
            sizeof(_mount_table[0]) * _mount_table_len);
    }

    // Find an empty slot
    for (new_element = 0; new_element < _mount_table_len; new_element++)
    {
        if (_mount_table[new_element].pathlen == 0)
        {
            break;
        }
    }

    if (new_element >= _mount_table_len)
    {
        new_element = _mount_table_len;
        _mount_table_len += _MOUNT_TABLE_SIZE_BUMP;
        _mount_table = (oe_mount_point_t*)oe_malloc(
            sizeof(_mount_table[0]) * _mount_table_len);
        oe_memset(_mount_table+new_element, 0, (sizeof(_mount_table[0]) * _MOUNT_TABLE_SIZE_BUMP));
    }

    _mount_table[new_element].mount_path = path;
    _mount_table[new_element].pathlen = oe_strlen(path);
    _mount_table[new_element].fs = oe_device_alloc(device_id, path, 0);

    // rslt = (*_mount_table.ops.fs-

    return 0;
}

int oe_open(const char* pathname, int flags, oe_mode_t mode)
{
    int fd = -1;
    oe_device_t* pfs = NULL;
    oe_device_t* pfile = NULL;
    char filepath[OE_PATH_MAX] = {0};

    if (!(pfs = _fs_lookup(pathname, filepath)))
    {
        return -1;
    }

    pfile = oe_clone_device(pfs);

    if ((fd = oe_allocate_fd()) < 0)
    {
        // Log error here
        return -1; // errno is already set
    }

    if (!(pfile = (*pfile->ops.fs->open)(pfile, filepath, flags, mode)))
    {
        oe_release_fd(fd);
        return -1;
    }

    if (!oe_set_fd_device(fd, pfile))
    {
        // Log error here
        return -1; // erno is already set
    }

    return fd;
}
