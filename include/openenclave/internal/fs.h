// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_FS_H
#define _OE_FS_H

#include <openenclave/internal/device.h>

OE_EXTERNC_BEGIN

/* The host calls this to install the host file system (HOSTFS). */
void oe_fs_install_hostfs(void);

/* The enclave calls this to get an instance of host file system (HOSTFS). */
oe_device_t* oe_fs_get_hostfs(void);

/* The host calls this to install the host file system (SGXFS). */
void oe_fs_install_sgxfs(void);

/* The enclave calls this to get an instance of host file system (SGXFS). */
oe_device_t* oe_fs_get_sgxfs(void);

int oe_register_hostfs_device(void);

int oe_register_sgxfs_device(void);

/* Set the default device for this thread (used in lieu of the mount table). */
int oe_set_thread_default_device(int device_id);

/* Clear the default device for this thread. */
int oe_clear_thread_default_device(void);

/* Get the default device for this thread. */
int oe_get_thread_default_device(void);

int oe_mount(int device_id, const char* path, uint32_t flags);

int oe_unmount(int device_id, const char* path);

int oe_open(const char* pathname, int flags, oe_mode_t mode);

oe_off_t oe_lseek(int fd, oe_off_t offset, int whence);

oe_device_t* oe_opendir(const char* pathname);

struct oe_dirent* oe_readdir(oe_device_t* dir);

int oe_closedir(oe_device_t* dir);

int oe_unlink(const char* pathname);

int oe_link(const char* oldpath, const char* newpath);

int oe_rename(const char* oldpath, const char* newpath);

int oe_mkdir(const char* pathname, oe_mode_t mode);

int oe_rmdir(const char* pathname);

int oe_stat(const char* pathname, struct oe_stat* buf);

int oe_truncate(const char* path, oe_off_t length);

ssize_t oe_readv(int fd, const oe_iovec_t* iov, int iovcnt);

ssize_t oe_writev(int fd, const oe_iovec_t* iov, int iovcnt);

int oe_access(const char* pathname, int mode);

oe_device_t* oe_opendir_dev(int device_id, const char* pathname);

struct oe_dirent* oe_readdir(oe_device_t* dirp);

int oe_closedir(oe_device_t* dirp);

int oe_unlink_dev(int device_id, const char* pathname);

int oe_link_dev(int device_id, const char* oldpath, const char* newpath);

int oe_rename_dev(int device_id, const char* oldpath, const char* newpath);

int oe_mkdir_dev(int device_id, const char* pathname, oe_mode_t mode);

int oe_rmdir_dev(int device_id, const char* pathname);

int oe_stat_dev(int device_id, const char* pathname, struct oe_stat* buf);

int oe_truncate_dev(int device_id, const char* path, oe_off_t length);

int oe_access_dev(int device_id, const char* pathname, int mode);

OE_EXTERNC_END

#endif // _OE_FS_H
