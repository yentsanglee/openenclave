// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _file_system_h
#define _file_system_h

#include <openenclave/enclave.h>
#include <openenclave/internal/fs.h>

class fs_file_system
{
  public:
    typedef oe_device_t* file_handle;
    typedef oe_device_t* dir_handle;

    fs_file_system(oe_device_t* fs) : _fs(fs)
    {
    }

    file_handle open(const char* pathname, int flags, oe_mode_t mode)
    {
        return (file_handle)_fs->ops.fs->open(_fs, pathname, flags, mode);
    }

    ssize_t write(file_handle file, const void* buf, size_t count)
    {
        return file->ops.fs->base.write((oe_device_t*)file, buf, count);
    }

    ssize_t read(file_handle file, void* buf, size_t count)
    {
        return file->ops.fs->base.read(file, buf, count);
    }

    oe_off_t lseek(file_handle file, oe_off_t offset, int whence)
    {
        return _fs->ops.fs->lseek(file, offset, whence);
    }

    int close(file_handle file)
    {
        return file->ops.fs->base.close((oe_device_t*)file);
    }

    dir_handle opendir(const char* name)
    {
        return _fs->ops.fs->opendir(_fs, name);
    }

    struct oe_dirent* readdir(dir_handle dir)
    {
        return dir->ops.fs->readdir(dir);
    }

    int closedir(dir_handle dir)
    {
        return dir->ops.fs->closedir(dir);
    }

    int unlink(const char* pathname)
    {
        return _fs->ops.fs->unlink(_fs, pathname);
    }

    int link(const char* oldpath, const char* newpath)
    {
        return _fs->ops.fs->link(_fs, oldpath, newpath);
    }

    int rename(const char* oldpath, const char* newpath)
    {
        return _fs->ops.fs->rename(_fs, oldpath, newpath);
    }

    int mkdir(const char* pathname, oe_mode_t mode)
    {
        return _fs->ops.fs->mkdir(_fs, pathname, mode);
    }

    int rmdir(const char* pathname)
    {
        return _fs->ops.fs->rmdir(_fs, pathname);
    }

    int stat(const char* pathname, struct oe_stat* buf)
    {
        return _fs->ops.fs->stat(_fs, pathname, buf);
    }

    int truncate(const char* path, oe_off_t length)
    {
        return _fs->ops.fs->truncate(_fs, path, length);
    }

  private:
    oe_device_t* _fs;
};

class hostfs_file_system : public fs_file_system
{
  public:
    hostfs_file_system() : fs_file_system(oe_fs_get_hostfs())
    {
    }
};

class sgxfs_file_system : public fs_file_system
{
  public:
    sgxfs_file_system() : fs_file_system(oe_fs_get_sgxfs())
    {
    }
};

class fd_file_system
{
  public:
    typedef int file_handle;
    typedef oe_device_t* dir_handle;

    fd_file_system(void)
    {
    }

    file_handle open(const char* pathname, int flags, oe_mode_t mode)
    {
        return (file_handle)oe_open(pathname, flags, mode);
    }

    ssize_t write(file_handle file, const void* buf, size_t count)
    {
        return oe_write(file, buf, count);
    }

    ssize_t read(file_handle file, void* buf, size_t count)
    {
        return oe_read(file, buf, count);
    }

    oe_off_t lseek(file_handle file, oe_off_t offset, int whence)
    {
        return oe_lseek(file, offset, whence);
    }

    int close(file_handle file)
    {
        return oe_close(file);
    }

    dir_handle opendir(const char* name)
    {
        return (dir_handle)oe_opendir(name);
    }

    struct oe_dirent* readdir(dir_handle dir)
    {
        return oe_readdir((oe_device_t*)dir);
    }

    int closedir(dir_handle dir)
    {
        return oe_closedir((oe_device_t*)dir);
    }

    int unlink(const char* pathname)
    {
        return oe_unlink(pathname);
    }

    int link(const char* oldpath, const char* newpath)
    {
        return oe_link(oldpath, newpath);
    }

    int rename(const char* oldpath, const char* newpath)
    {
        return oe_rename(oldpath, newpath);
    }

    int mkdir(const char* pathname, oe_mode_t mode)
    {
        return oe_mkdir(pathname, mode);
    }

    int rmdir(const char* pathname)
    {
        return oe_rmdir(pathname);
    }

    int stat(const char* pathname, struct oe_stat* buf)
    {
        return oe_stat(pathname, buf);
    }

    int truncate(const char* path, oe_off_t length)
    {
        return oe_truncate(path, length);
    }

  private:
};

#endif /* _file_system_h */
