// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_MOUNT_H
#define _FS_MOUNT_H

#include "common.h"

FS_EXTERN_C_BEGIN

typedef int (*fs_mount_callback_t)(
    const char* type,
    const char* source,
    const char* target,
    va_list ap);

int fs_register(const char* type, fs_mount_callback_t callback);
    
int fs_mount(const char* type, const char* source, const char* target, ...);

int fs_unmount(const char* target);

FS_EXTERN_C_END

#endif /* _FS_MOUNT_H */
