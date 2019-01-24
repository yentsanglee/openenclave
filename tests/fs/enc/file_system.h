// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _file_system_h
#define _file_system_h

#include <openenclave/enclave.h>
#include <openenclave/internal/fs.h>

template<class T>
class file_system
{
};

template<>
class file_system<oe_device_t>
{
public:

    typedef oe_device_t* file_handle;
    typedef oe_device_t* dir_handle;

    file_system(oe_device_t* fs) : _fs(fs)
    {
    }

    ~file_system()
    {
        oe_fs_release(_fs);
    }

    file_handle open(const char* pathname, int flags, oe_mode_t mode)
    {
        return oe_fs_open(_fs, pathname, flags, mode);
    }

    ssize_t write(file_handle file, const void* buf, size_t count)
    {
        return oe_fs_write(file, buf, count);
    }

    ssize_t read(file_handle file, void* buf, size_t count)
    {
        return oe_fs_read(file, buf, count);
    }

    oe_off_t lseek(file_handle file, oe_off_t offset, int whence)
    {
        return oe_fs_lseek(file, offset, whence);
    }

    int close(file_handle file)
    {
        return oe_fs_close(file);
    }

    oe_device_t* opendir(const char* name)
    {
        return oe_fs_opendir(_fs, name);
    }

    struct oe_dirent* readdir(oe_device_t* dir)
    {
        return oe_fs_readdir(dir);
    }

    int closedir(oe_device_t* dir)
    {
        return oe_fs_closedir(dir);
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

#endif /* _file_system_h */
