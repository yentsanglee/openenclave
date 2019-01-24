// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_FS_H
#define _OE_FS_H

#include <openenclave/internal/device.h>

OE_EXTERNC_BEGIN

#define FS_MAGIC 0x5f35f964
#define FILE_MAGIC 0xfe48c6ff
#define DIR_MAGIC 0x8add1b0b

typedef struct _fs
{
    struct _oe_device base;
    uint32_t magic;
} fs_t;

typedef struct _file
{
    struct _oe_device base;
    uint32_t magic;
    int host_fd;
} file_t;

typedef struct _dir
{
    struct _oe_device base;
    uint32_t magic;
    void* host_dir;
    struct oe_dirent entry;
} dir_t;

OE_INLINE fs_t* _cast_fs(const oe_device_t device)
{
    fs_t* fs = (fs_t*)device;

    if (fs == NULL || fs->magic != FS_MAGIC)
        return NULL;

    return fs;
}

OE_INLINE file_t* _cast_file(const oe_device_t device)
{
    file_t* file = (file_t*)device;

    if (file == NULL || file->magic != FILE_MAGIC)
        return NULL;

    return file;
}

OE_INLINE dir_t* _cast_dir(const oe_device_t device)
{
    dir_t* dir = (dir_t*)device;

    if (dir == NULL || dir->magic != DIR_MAGIC)
        return NULL;

    return dir;
}

/* The host calls this to install the host file system (HOSTFS). */
void oe_fs_install_hostfs(void);

/* The enclave calls this to get an instance of host file system (HOSTFS). */
oe_device_t oe_fs_get_hostfs(void);

OE_INLINE oe_device_t* oe_fs_open(
    oe_device_t* fs,
    const char* pathname,
    int flags,
    oe_mode_t mode)
{
    return fs->ops.fs->open(fs, pathname, flags, mode);
}

OE_INLINE ssize_t oe_fs_read(oe_device_t* file, void* buf, size_t count)
{
    return file->ops.fs->base.read(file, buf, count);
}

OE_INLINE ssize_t oe_fs_write(oe_device_t* file, const void* buf, size_t count)
{
    return file->ops.fs->base.write(file, buf, count);
}

OE_INLINE oe_off_t oe_fs_lseek(oe_device_t* file, oe_off_t offset, int whence)
{
    return file->ops.fs->lseek(file, offset, whence);
}

OE_INLINE int oe_fs_ioctl(
    oe_device_t* file,
    unsigned long request,
    oe_va_list ap)
{
    return file->ops.fs->base.ioctl(file, request, ap);
}

OE_INLINE int oe_fs_close(oe_device_t* file)
{
    return file->ops.fs->base.close(file);
}

OE_INLINE oe_device_t* oe_fs_opendir(oe_device_t* fs, const char* name)
{
    return (*fs->ops.fs->opendir)(fs, name);
}

OE_INLINE struct oe_dirent* oe_fs_readdir(oe_device_t* dir)
{
    return (*dir->ops.fs->readdir)(dir);
}

OE_INLINE int oe_fs_closedir(oe_device_t* dir)
{
    return (*dir->ops.fs->closedir)(dir);
}

OE_INLINE int oe_fs_stat(
    oe_device_t* fs,
    const char* pathname,
    struct oe_stat* buf)
{
    return fs->ops.fs->stat(fs, pathname, buf);
}

OE_INLINE int oe_fs_link(
    oe_device_t* fs,
    const char* oldpath,
    const char* newpath)
{
    return fs->ops.fs->link(fs, oldpath, newpath);
}

OE_INLINE int oe_fs_unlink(oe_device_t* fs, const char* pathname)
{
    return fs->ops.fs->unlink(fs, pathname);
}

OE_INLINE int oe_fs_rename(
    oe_device_t* fs,
    const char* oldpath,
    const char* newpath)
{
    return fs->ops.fs->rename(fs, oldpath, newpath);
}

OE_INLINE int oe_fs_truncate(oe_device_t* fs, const char* path, oe_off_t length)
{
    return fs->ops.fs->truncate(fs, path, length);
}

OE_INLINE int oe_fs_mkdir(oe_device_t* fs, const char* pathname, oe_mode_t mode)
{
    return fs->ops.fs->mkdir(fs, pathname, mode);
}

OE_INLINE int oe_fs_rmdir(oe_device_t* fs, const char* pathname)
{
    return fs->ops.fs->rmdir(fs, pathname);
}

OE_EXTERNC_END

#endif // _OE_FS_H
