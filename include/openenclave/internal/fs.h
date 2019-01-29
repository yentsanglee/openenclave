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

OE_EXTERNC_END

#endif // _OE_FS_H
