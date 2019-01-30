// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <openenclave/enclave.h>
#include <openenclave/internal/thread.h>
// clang-format on

#include <openenclave/internal/fs.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/atexit.h>

#define MAX_MOUNT_TABLE_SIZE 64

/* ATTN: define and enforce mounting flags. */

typedef struct _mount_point
{
    char* path;
    oe_device_t* fs;
    uint32_t flags;
} mount_point_t;

static mount_point_t _mount_table[MAX_MOUNT_TABLE_SIZE];
size_t _mount_table_size = 0;
static oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;

static bool _installed_free_mount_table = false;

static void _free_mount_table(void)
{
    for (size_t i = 0; i < _mount_table_size; i++)
        oe_free(_mount_table[i].path);
}

static oe_device_t* _fs_lookup(const char* path, char suffix[OE_PATH_MAX])
{
    oe_device_t* ret = NULL;
    size_t match_len = 0;

    oe_spin_lock(&_lock);
    {
        /* Find the longest binding point that contains this path. */
        for (size_t i = 0; i < _mount_table_size; i++)
        {
            size_t len = oe_strlen(_mount_table[i].path);
            const char* mpath = _mount_table[i].path;

            if (mpath[0] == '/' && mpath[1] == '\0')
            {
                if (len > match_len)
                {
                    if (suffix)
                    {
                        oe_strlcpy(suffix, path, OE_PATH_MAX);
                    }

                    match_len = len;
                    ret = _mount_table[i].fs;
                }
            }
            else if (oe_strncmp(mpath, path, len) == 0 &&
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
    oe_spin_unlock(&_lock);

    return ret;
}

int oe_mount(int device_id, const char* path, uint32_t flags)
{
    int ret = -1;
    oe_device_t* device = oe_get_devid_device(device_id);
    oe_device_t* new_device = NULL;

    OE_UNUSED(flags);

    oe_spin_lock(&_lock);

    if (!device || device->type != OE_DEVICETYPE_FILESYSTEM || !path)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Install _free_mount_table() if not already installed. */
    if (_installed_free_mount_table == false)
    {
        oe_atexit(_free_mount_table);
        _installed_free_mount_table = true;
    }

    /* Fail if mount table exhausted. */
    if (_mount_table_size == MAX_MOUNT_TABLE_SIZE)
    {
        oe_errno = OE_ENOMEM;
        goto done;
    }

    /* Reject duplicate mount paths. */
    for (size_t i = 0; i < _mount_table_size; i++)
    {
        if (oe_strcmp(_mount_table[i].path, path) == 0)
        {
            oe_errno = OE_EEXIST;
            goto done;
        }
    }

    /* Clone the device. */
    if (device->ops.fs->base.clone(device, &new_device) != 0)
    {
        oe_errno = OE_ENOMEM;
        goto done;
    }

    /* Assign and initialize new mount point. */
    {
        size_t index = _mount_table_size;
        size_t path_len = oe_strlen(path);

        if (!(_mount_table[index].path = oe_malloc(path_len + 1)))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        oe_memcpy(_mount_table[index].path, path, path_len + 1);
        _mount_table[index].fs = new_device;
        _mount_table_size++;
    }

    new_device = NULL;
    ret = 0;

done:

    if (new_device)
        new_device->ops.fs->base.release(new_device);

    oe_spin_unlock(&_lock);
    return ret;
}

int oe_unmount(int device_id, const char* path)
{
    int ret = -1;
    size_t index = (size_t)-1;
    oe_device_t* device = oe_get_devid_device(device_id);

    oe_spin_lock(&_lock);

    if (!device || device->type != OE_DEVICETYPE_FILESYSTEM || !path)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Find and remove this device. */
    for (size_t i = 0; i < _mount_table_size; i++)
    {
        if (oe_strcmp(_mount_table[i].path, path) == 0)
        {
            index = i;
            break;
        }
    }

    /* If mount point not found. */
    if (index == (size_t)-1)
    {
        oe_errno = OE_ENOENT;
        goto done;
    }

    /* Remove the entry by swapping with the last entry. */
    {
        oe_device_t* fs = _mount_table[index].fs;

        oe_free(_mount_table[index].path);
        fs = _mount_table[index].fs;
        _mount_table[index] = _mount_table[_mount_table_size-1];
        _mount_table_size--;

        fs->ops.fs->base.release(fs);
    }

    ret = 0;

done:

    oe_spin_unlock(&_lock);
    return ret;
}

int oe_open(const char* pathname, int flags, oe_mode_t mode)
{
    int ret = -1;
    int fd = -1;
    oe_device_t* fs;
    oe_device_t* file;
    char filepath[OE_PATH_MAX] = {0};

    if (!pathname)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (!(fs = _fs_lookup(pathname, filepath)))
    {
        oe_errno = OE_ENOENT;
        goto done;
    }

    if ((fd = oe_allocate_fd()) < 0)
    {
        // oe_errno set by function.
        goto done;
    }

    if (!(file = (*fs->ops.fs->open)(fs, filepath, flags, mode)))
    {
        // oe_errno set by function.
        goto done;
    }

    if (!oe_set_fd_device(fd, file))
    {
        // oe_errno set by function.
        goto done;
    }

    ret = fd;
    fd = -1;

done:

    /* ATTN: release file. */

    if (fd != -1)
        oe_release_fd(fd);

    return ret;
}

oe_device_t* oe_opendir(const char* pathname)
{
    oe_device_t* fs = NULL;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = _fs_lookup(pathname, filepath)))
    {
        oe_errno = OE_EBADF;
        return NULL;
    }

    if (fs->type != OE_DEVICETYPE_FILESYSTEM)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    if (fs->ops.fs->opendir == NULL)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    return (*fs->ops.fs->opendir)(fs, filepath);
}

struct oe_dirent* oe_readdir(oe_device_t* dir)
{
    if (dir->type != OE_DEVICETYPE_DIRECTORY)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    if (dir->ops.fs->readdir == NULL)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    return (*dir->ops.fs->readdir)(dir);
}

int oe_closedir(oe_device_t* dir)
{
    if (dir->type != OE_DEVICETYPE_DIRECTORY)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    if (dir->ops.fs->closedir == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return (*dir->ops.fs->closedir)(dir);
}

int oe_rmdir(const char* pathname)
{
    oe_device_t* fs = NULL;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = _fs_lookup(pathname, filepath)))
    {
        return -1;
    }

    if (fs->ops.fs->rmdir == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return (*fs->ops.fs->rmdir)(fs, filepath);
}

int oe_stat(const char* pathname, struct oe_stat* buf)
{
    oe_device_t* fs = NULL;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = _fs_lookup(pathname, filepath)))
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (fs->ops.fs->stat == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return (*fs->ops.fs->stat)(fs, filepath, buf);
}

int oe_link(const char* oldpath, const char* newpath)
{
    oe_device_t* fs = NULL;
    oe_device_t* newfs = NULL;
    char filepath[OE_PATH_MAX] = {0};
    char newfilepath[OE_PATH_MAX] = {0};

    if (!(fs = _fs_lookup(oldpath, filepath)))
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (!(newfs = _fs_lookup(newpath, newfilepath)))
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (fs != newfs)
    {
        oe_errno = OE_EXDEV;
        return -1;
    }

    if (fs->ops.fs->link == NULL)
    {
        oe_errno = OE_EPERM;
        return -1;
    }

    return (*fs->ops.fs->link)(fs, filepath, newfilepath);
}

int oe_unlink(const char* pathname)
{
    oe_device_t* fs = NULL;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = _fs_lookup(pathname, filepath)))
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (fs->ops.fs->unlink == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return (*fs->ops.fs->unlink)(fs, filepath);
}

int oe_rename(const char* oldpath, const char* newpath)
{
    oe_device_t* fs = NULL;
    oe_device_t* newfs = NULL;
    char filepath[OE_PATH_MAX] = {0};
    char newfilepath[OE_PATH_MAX] = {0};

    if (!(fs = _fs_lookup(oldpath, filepath)))
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (!(newfs = _fs_lookup(newpath, newfilepath)))
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (fs != newfs)
    {
        oe_errno = OE_EXDEV;
        return -1;
    }

    if (fs->ops.fs->rename == NULL)
    {
        oe_errno = OE_EPERM;
        return -1;
    }

    return (*fs->ops.fs->rename)(fs, filepath, newfilepath);
}

int oe_truncate(const char* pathname, oe_off_t length)
{
    oe_device_t* fs = NULL;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = _fs_lookup(pathname, filepath)))
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (fs->ops.fs->truncate == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return (*fs->ops.fs->truncate)(fs, filepath, length);
}

int oe_mkdir(const char* pathname, oe_mode_t mode)
{
    oe_device_t* fs = NULL;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = _fs_lookup(pathname, filepath)))
    {
        oe_errno = OE_EBADF;
        return -1;
    }

    if (fs->ops.fs->mkdir == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return (*fs->ops.fs->mkdir)(fs, filepath, mode);
}

oe_off_t oe_lseek(int fd, oe_off_t offset, int whence)
{
    oe_device_t* file = oe_get_fd_device(fd);
    if (!file)
    {
        // Log error here
        oe_errno = OE_EBADF;
        return -1;
    }

    if (file->ops.fs->lseek == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return (*file->ops.fs->lseek)(file, offset, whence);
}

ssize_t oe_readv(int fd, const oe_iovec_t* iov, int iovcnt)
{
    ssize_t ret = -1;
    ssize_t nread = 0;

    if (fd < 0 || !iov)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    for (int i = 0; i < iovcnt; i++)
    {
        const oe_iovec_t* p = &iov[i];
        ssize_t n;

        n = oe_read(fd, p->iov_base, p->iov_len);

        if (n < 0)
            goto done;

        nread += n;

        if ((size_t)n < p->iov_len)
            break;
    }

    ret = nread;

done:
    return ret;
}

ssize_t oe_writev(int fd, const oe_iovec_t* iov, int iovcnt)
{
    ssize_t ret = -1;
    ssize_t nwritten = 0;

    if (fd < 0 || !iov)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    for (int i = 0; i < iovcnt; i++)
    {
        const oe_iovec_t* p = &iov[i];
        ssize_t n;

        n = oe_write(fd, p->iov_base, p->iov_len);

        if ((size_t)n != p->iov_len)
        {
            oe_errno = OE_EIO;
            goto done;
        }

        nwritten += n;
    }

    ret = nwritten;

done:
    return ret;
}

int oe_access(const char *pathname, int mode)
{
    int ret = -1;
    struct oe_stat buf;

    OE_UNUSED(mode);

    if (!pathname)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (oe_stat(pathname, &buf) != 0)
    {
        oe_errno = OE_ENOENT;
        goto done;
    }

done:
    return ret;
}
