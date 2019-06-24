// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "register.h"
#include "syscall.h"

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

void ve_set_fs_register_base(const void* ptr)
{
    ve_syscall2(OE_SYS_arch_prctl, ARCH_SET_FS, (long)ptr);
}

void* ve_get_fs_register_base(void)
{
    void* ptr = NULL;
    ve_syscall2(OE_SYS_arch_prctl, ARCH_GET_FS, (long)&ptr);
    return ptr;
}
