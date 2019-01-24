// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/device.h>

struct _mount_point
{
    size_t pathlen; // an optimisation. I can easily skip paths that are the
                    // wrong length without needing to do a string compare.
    const char* mount_path;
    oe_device_t filesystem;
    uint32_t flags;
} * oe_mount_point_t;

const size_t _MOUNT_TABLE_SIZE_BUMP = 5;

size_t _mount_table_len = 0;
oe_mount_point_t* _mount_table = NULL;

static oe_device_t _find_fs_by_mount(const char* path)

{
    // check for dups.
   for (char *cp;
}

int oe_mount(int device_id, const char* path, uint32_t flags)

{
    size_t mount_path_len = -1;
    size_t new_element = 0;
    fs_t* pfs_device = oe_getdevid_device(device_id);
    int new_devid = -1;

    if (pfs_device == NULL)
    {
        oe_errno = OE_EBADFD;
        return -1;
    }

    if (pfs_device->magic != FS_MAGIC)
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
        _mount_table = (oe_mount_point_t)oe_malloc(
            sizeof(_mount_table[0]) * _mount_table_len);
        oe_memset(_mount_table, 0, (sizeof(_mount_table[0]) * _mount_table_len);
    }

    // Find an empty slot
    for (new_element = 0; new_element < _mount_table_len; new_element++)
    {
        if (_mount_table[new_element].pathlen = 0)
        {
            break;
        }
    }

    if (new_element >= _mount_table_len)
    {
        new_element = _mount_table_len;
        _mount_table_len += _MOUNT_TABLE_SIZE_BUMP;
        _mount_table = (oe_mount_point_t)oe_malloc(
            sizeof(_mount_table[0]) * _mount_table_len);
        oe_memset(_mount_table+new_element, 0, (sizeof(_mount_table[0]) * _MOUNT_TABLE_SIZE_BUMP);
    }

    _mount_table.mount_path = path;
    _mount_table.pathlen = strlen(path);
    _mount_table.file_system = oe_device_alloc(device_id, path, 0);
   rslt = (*_mount_table.ops.fs-
   return 0;
}

int oe_open(const char* pathname, int flags, mode_t mode)

{
    int fd = -1;
    fs_t* pfs = NULL;
    file_t* pfile = NULL;

    pfs = _find_fs_by_path(pathname);
    pfile =

        fd = oe_allocate_fd();
    if (sd < 0)
    {
        // Log error here
        return -1; // errno is already set
    }
    psock = oe_set_fd_device(sd, psock);
    if (!psock)
    {
        // Log error here
        return -1; // erno is already set
    }

    if ((*psock->ops.socket->socket)(psock, domain, type, protocol) < 0)
    {
        oe_release_fd(sd);
        return -1;
    }
    return sd;
}
