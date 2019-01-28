// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _file_system_h
#define _file_system_h

#include <openenclave/enclave.h>
#include <openenclave/internal/fs.h>

class fs_file_system
{
  public:
    typedef struct _fs_file_system_file_handle* file_handle;
    typedef struct _fs_file_system_dir_handle* dir_handle;

    fs_file_system(oe_device_t* fs) : _fs(fs)
    {
    }

    file_handle open(const char* pathname, int flags, oe_mode_t mode)
    {
        return (file_handle)oe_fs_open(_fs, pathname, flags, mode);
    }

    ssize_t write(file_handle file, const void* buf, size_t count)
    {
        return oe_fs_write((oe_device_t*)file, buf, count);
    }

    ssize_t read(file_handle file, void* buf, size_t count)
    {
        return oe_fs_read((oe_device_t*)file, buf, count);
    }

    oe_off_t lseek(file_handle file, oe_off_t offset, int whence)
    {
        return oe_fs_lseek((oe_device_t*)file, offset, whence);
    }

    int close(file_handle file)
    {
        return oe_fs_close((oe_device_t*)file);
    }

    dir_handle opendir(const char* name)
    {
        return (dir_handle)oe_fs_opendir(_fs, name);
    }

    struct oe_dirent* readdir(dir_handle dir)
    {
        return oe_fs_readdir((oe_device_t*)dir);
    }

    int closedir(dir_handle dir)
    {
        return oe_fs_closedir((oe_device_t*)dir);
    }

    int unlink(const char* pathname)
    {
        return oe_fs_unlink(_fs, pathname);
    }

    int link(const char* oldpath, const char* newpath)
    {
        return oe_fs_link(_fs, oldpath, newpath);
    }

    int rename(const char* oldpath, const char* newpath)
    {
        return oe_fs_rename(_fs, oldpath, newpath);
    }

    int mkdir(const char* pathname, oe_mode_t mode)
    {
        return oe_fs_mkdir(_fs, pathname, mode);
    }

    int rmdir(const char* pathname)
    {
        return oe_fs_rmdir(_fs, pathname);
    }

    int stat(const char* pathname, struct oe_stat* buf)
    {
        return oe_fs_stat(_fs, pathname, buf);
    }

    int truncate(const char* path, oe_off_t length)
    {
        return oe_fs_truncate(_fs, path, length);
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

#endif /* _file_system_h */
