// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include <openenclave/enclave.h>
#include <openenclave/internal/thread.h>
// clang-format on

#include <openenclave/bits/fs.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/atexit.h>
#include <openenclave/internal/print.h>

#define MAX_MOUNT_TABLE_SIZE 64

/* ATTN: define and enforce mounting flags. */

typedef struct _mount_point
{
    char* path;
    oe_device_t* fs;
    uint32_t flags;
} mount_point_t;

typedef struct _path
{
    char buf[OE_PATH_MAX];
}
path_t;

static mount_point_t _mount_table[MAX_MOUNT_TABLE_SIZE];
size_t _mount_table_size = 0;
static oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;

static bool _installed_free_mount_table = false;

static oe_once_t _device_id_once = OE_ONCE_INIT;
static oe_thread_key_t _device_id_key = OE_THREADKEY_INITIALIZER;

static char _chroot_path[OE_PATH_MAX] = "/";
static oe_spinlock_t _chroot_lock = OE_SPINLOCK_INITIALIZER;

static void _free_mount_table(void)
{
    for (size_t i = 0; i < _mount_table_size; i++)
        oe_free(_mount_table[i].path);
}

static void _get_chroot_path(path_t* path)
{
    oe_spin_lock(&_chroot_lock);
    oe_strlcpy(path->buf, _chroot_path, sizeof(path_t));
    oe_spin_unlock(&_chroot_lock);
}

static int _get_full_path(path_t* full_path, const char* path)
{
    int ret = -1;
    path_t chroot_path;
    size_t len1;
    size_t len2;

    if (!full_path || !path || path[0] != '/')
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    _get_chroot_path(&chroot_path);

    if (chroot_path.buf[0] == '/' && chroot_path.buf[1] == '\0')
        len1 = 0;
    else
        len1 = oe_strlen(chroot_path.buf);

    len2 = oe_strlen(path);

    if (len1 + len2 >= OE_PATH_MAX)
    {
        oe_errno = OE_ENAMETOOLONG;
        goto done;
    }

    full_path->buf[0] = '\0';

    if (len1)
        oe_strlcat(full_path->buf, chroot_path.buf, sizeof(path_t));

    oe_strlcat(full_path->buf, path, sizeof(path_t));

    ret = 0;

done:
    return ret;
}

static oe_device_t* _fs_lookup(const char* path, char suffix[OE_PATH_MAX])
{
    oe_device_t* ret = NULL;
    size_t match_len = 0;
    path_t full_path;

    /* First check whether a device id is set for this thread. */
    {
        oe_devid_t devid;

        if ((devid = oe_get_thread_default_device()) != OE_DEVID_NULL)
        {
            oe_device_t* device = oe_get_devid_device(devid);

            if (!device || device->type != OE_DEVICETYPE_FILESYSTEM)
                goto done;

            /* Use this device. */
            oe_strlcpy(suffix, path, OE_PATH_MAX);
            ret = device;
            goto done;
        }
    }

    /* Get the full path including the chroot part. */
    if (_get_full_path(&full_path, path) != 0)
    {
        goto done;
    }

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
                        oe_strlcpy(suffix, full_path.buf, OE_PATH_MAX);
                    }

                    match_len = len;
                    ret = _mount_table[i].fs;
                }
            }
            else if (
                oe_strncmp(mpath, full_path.buf, len) == 0 &&
                (full_path.buf[len] == '/' || full_path.buf[len] == '\0'))
            {
                if (len > match_len)
                {
                    if (suffix)
                    {
                        oe_strlcpy(suffix, full_path.buf + len, OE_PATH_MAX);

                        if (*suffix == '\0')
                            oe_strlcpy(suffix, "/", OE_PATH_MAX);
                    }

                    match_len = len;
                    ret = _mount_table[i].fs;
                }
            }
        }
    }
    oe_spin_unlock(&_lock);

done:
    return ret;
}

static oe_device_t* _get_fs_device(oe_devid_t devid)
{
    oe_device_t* device = oe_get_devid_device(devid);

    if (!device || device->type != OE_DEVICETYPE_FILESYSTEM)
        return NULL;

    return device;
}

static void _create_device_id_key()
{
    if (oe_thread_key_create(&_device_id_key, NULL) != 0)
        oe_abort();
}

int oe_set_thread_default_device(oe_devid_t devid)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
        goto done;

    if (oe_once(&_device_id_once, _create_device_id_key) != 0)
        goto done;

    if (oe_thread_setspecific(_device_id_key, (void*)devid) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

int oe_clear_thread_default_device(void)
{
    int ret = -1;

    if (oe_once(&_device_id_once, _create_device_id_key) != 0)
        goto done;

    if (oe_thread_setspecific(_device_id_key, NULL) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

oe_devid_t oe_get_thread_default_device(void)
{
    oe_devid_t ret = OE_DEVID_NULL;
    oe_devid_t devid;

    if (oe_once(&_device_id_once, _create_device_id_key) != 0)
        goto done;

    if (!(devid = (uint64_t)oe_thread_getspecific(_device_id_key)))
        goto done;

    ret = devid;

done:
    return ret;
}

int oe_chroot(const char* path)
{
    int ret = -1;

    if (!path)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (oe_strlen(path) >= sizeof(_chroot_path))
    {
        oe_errno = OE_ENAMETOOLONG;
        goto done;
    }

    /* Verify that the path refers to a directory. */
    {
        struct oe_stat buf;

        if (oe_stat(0, path, &buf) != 0)
        {
            oe_errno = OE_EIO;
            goto done;
        }

        if (!OE_S_ISDIR(buf.st_mode))
        {
            oe_errno = OE_ENOTDIR;
            goto done;
        }
    }

    oe_spin_lock(&_chroot_lock);
    oe_strlcpy(_chroot_path, path, sizeof(_chroot_path));
    oe_spin_unlock(&_chroot_lock);

    ret = 0;

done:
    return ret;
}

int oe_mount(
    oe_devid_t devid,
    const char* source,
    const char* target,
    uint32_t flags)
{
    int ret = -1;
    oe_device_t* device = oe_get_devid_device(devid);
    oe_device_t* new_device = NULL;
    path_t full_target;
    bool locked = false;

    if (!device || device->type != OE_DEVICETYPE_FILESYSTEM || !target)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Be sure the full_target directory exists (if not root). */
    if (oe_strcmp(target, "/") != 0)
    {
        struct oe_stat buf;

        if (oe_stat(0, target, &buf) != 0)
        {
            oe_errno = OE_EIO;
            goto done;
        }

        if (!OE_S_ISDIR(buf.st_mode))
        {
            oe_errno = OE_ENOTDIR;
            goto done;
        }
    }

    /* Get the full target including chroot part. */
    if (_get_full_path(&full_target, target) != 0)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Lock the mount table. */
    oe_spin_lock(&_lock);
    locked = true;

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
        if (oe_strcmp(_mount_table[i].path, full_target.buf) == 0)
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
        size_t len = oe_strlen(full_target.buf);

        if (!(_mount_table[index].path = oe_malloc(len + 1)))
        {
            oe_errno = OE_ENOMEM;
            goto done;
        }

        oe_memcpy(_mount_table[index].path, full_target.buf, len + 1);
        _mount_table[index].fs = new_device;
        _mount_table_size++;
    }

    /* Notify the device that it has been mounted. */
    if (new_device->ops.fs->mount(
        new_device, source, full_target.buf, flags) != 0)
    {
        oe_free(_mount_table[--_mount_table_size].path);
        goto done;
    }

    new_device = NULL;
    ret = 0;

done:

    if (new_device)
        new_device->ops.fs->base.release(new_device);

    if (locked)
        oe_spin_unlock(&_lock);

    return ret;
}

int oe_unmount(oe_devid_t devid, const char* target)
{
    int ret = -1;
    size_t index = (size_t)-1;
    oe_device_t* device = oe_get_devid_device(devid);

    oe_spin_lock(&_lock);

    if (!device || device->type != OE_DEVICETYPE_FILESYSTEM || !target)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    /* Find and remove this device. */
    for (size_t i = 0; i < _mount_table_size; i++)
    {
        if (oe_strcmp(_mount_table[i].path, target) == 0)
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
        _mount_table[index] = _mount_table[_mount_table_size - 1];
        _mount_table_size--;

        if (fs->ops.fs->unmount(fs, target) != 0)
            goto done;

        fs->ops.fs->base.release(fs);
    }

    ret = 0;

done:

    oe_spin_unlock(&_lock);
    return ret;
}

static int _open(const char* pathname, int flags, mode_t mode)
{
    int ret = -1;
    int fd;
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

    if (!(file = (*fs->ops.fs->open)(fs, filepath, flags, mode)))
    {
        // oe_errno set by function.
        goto done;
    }

    if ((fd = oe_assign_fd_device(file)) == -1)
    {
        (*fs->ops.fs->base.close)(file);
        goto done;
    }

    ret = fd;

done:

    return ret;
}

int oe_open(oe_devid_t devid, const char* pathname, int flags, mode_t mode)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _open(pathname, flags, mode);
    }
    else
    {
        oe_device_t* dev;
        oe_device_t* file;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (!(file = (*dev->ops.fs->open)(dev, pathname, flags, mode)))
            goto done;

        if ((ret = oe_assign_fd_device(file)) == -1)
        {
            (*dev->ops.fs->base.close)(file);
            goto done;
        }
    }

done:
    return ret;
}

static OE_DIR* _opendir(const char* pathname)
{
    oe_device_t* fs = NULL;
    char filepath[OE_PATH_MAX] = {0};

oe_host_printf("_opendir{%s}\n", pathname);

    if (!(fs = _fs_lookup(pathname, filepath)))
    {
        oe_errno = OE_EBADF;
        return NULL;
    }

oe_host_printf("filepath{%s}\n", filepath);

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

    return (OE_DIR*)(*fs->ops.fs->opendir)(fs, filepath);
}

struct oe_dirent* oe_readdir(OE_DIR* dir)
{
    oe_device_t* dev = (oe_device_t*)dir;

    if (dev->type != OE_DEVICETYPE_DIRECTORY)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    if (dev->ops.fs->readdir == NULL)
    {
        oe_errno = OE_EINVAL;
        return NULL;
    }

    return (*dev->ops.fs->readdir)(dev);
}

int oe_closedir(OE_DIR* dir)
{
    oe_device_t* dev = (oe_device_t*)dir;

    if (dev->type != OE_DEVICETYPE_DIRECTORY)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    if (dev->ops.fs->closedir == NULL)
    {
        oe_errno = OE_EINVAL;
        return -1;
    }

    return (*dev->ops.fs->closedir)(dev);
}

static int _rmdir(const char* pathname)
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

static int _stat(const char* pathname, struct oe_stat* buf)
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

static int _link(const char* oldpath, const char* newpath)
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

static int _unlink(const char* pathname)
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

static int _rename(const char* oldpath, const char* newpath)
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

static int _truncate(const char* pathname, off_t length)
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

static int _mkdir(const char* pathname, mode_t mode)
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

off_t oe_lseek(int fd, off_t offset, int whence)
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

static int _access(const char* pathname, int mode)
{
    int ret = -1;
    struct oe_stat buf;

    OE_UNUSED(mode);

    if (!pathname)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (_stat(pathname, &buf) != 0)
    {
        oe_errno = OE_ENOENT;
        goto done;
    }

done:
    return ret;
}

OE_DIR* oe_opendir(oe_devid_t devid, const char* pathname)
{
    OE_DIR* ret = NULL;

    if (devid == OE_DEVID_NULL)
    {
        ret = _opendir(pathname);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = (OE_DIR*)dev->ops.fs->opendir(dev, pathname);
    }

done:
    return ret;
}

int oe_unlink(oe_devid_t devid, const char* pathname)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _unlink(pathname);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->unlink(dev, pathname);
    }

done:
    return ret;
}

int oe_link(oe_devid_t devid, const char* oldpath, const char* newpath)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _link(oldpath, newpath);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->link(dev, oldpath, newpath);
    }

done:
    return ret;
}

int oe_rename(oe_devid_t devid, const char* oldpath, const char* newpath)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _rename(oldpath, newpath);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->rename(dev, oldpath, newpath);
    }

done:
    return ret;
}

int oe_mkdir(oe_devid_t devid, const char* pathname, mode_t mode)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _mkdir(pathname, mode);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->mkdir(dev, pathname, mode);
    }

done:
    return ret;
}

int oe_rmdir(oe_devid_t devid, const char* pathname)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _rmdir(pathname);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->rmdir(dev, pathname);
    }

done:
    return ret;
}

int oe_stat(oe_devid_t devid, const char* pathname, struct oe_stat* buf)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _stat(pathname, buf);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->stat(dev, pathname, buf);
    }

done:
    return ret;
}

int oe_truncate(oe_devid_t devid, const char* path, off_t length)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _truncate(path, length);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->truncate(dev, path, length);
    }

done:
    return ret;
}

int oe_access(oe_devid_t devid, const char* pathname, int mode)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _access(pathname, mode);
    }
    else
    {
        oe_device_t* dev;
        struct oe_stat buf;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (!pathname)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (_stat(pathname, &buf) != 0)
        {
            oe_errno = OE_ENOENT;
            goto done;
        }

        ret = 0;
    }

done:
    return ret;
}
