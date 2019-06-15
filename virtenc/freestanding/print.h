// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FREESTANDING_PRINT_H
#define _FREESTANDING_PRINT_H

#include "../freestanding/defs.h"
#include "../freestanding/types.h"
#include "../freestanding/syscall.h"
#include "../freestanding/string.h"

FS_INLINE void fs_print_str(const char* s)
{
    fs_syscall3(FS_SYS_write, 1, (long)s, fs_strlen(s));
}

FS_INLINE void fs_print_nl(void)
{
    const char nl = '\n';
    fs_syscall3(FS_SYS_write, 1, (long)&nl, 1);
}

FS_INLINE void fs_print_oct(uint64_t x)
{
    fs_intstr_buf_t buf;
    fs_print_str(fs_uint64_octstr(&buf, x, NULL));
}

FS_INLINE void fs_print_uint(uint64_t x)
{
    fs_intstr_buf_t buf;
    fs_print_str(fs_uint64_decstr(&buf, x, NULL));
}

FS_INLINE void fs_print_int(int64_t x)
{
    fs_intstr_buf_t buf;
    fs_print_str(fs_int64_decstr(&buf, x, NULL));
}

FS_INLINE void fs_print_hex(uint64_t x)
{
    fs_intstr_buf_t buf;
    fs_print_str(fs_uint64_hexstr(&buf, x, NULL));
}

#endif /* _FREESTANDING_PRINT_H */
