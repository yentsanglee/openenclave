// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FREESTANDING_PRINT_H
#define _FREESTANDING_PRINT_H

#include "../freestanding/defs.h"
#include "../freestanding/types.h"
#include "../freestanding/syscall.h"
#include "../freestanding/string.h"

FS_INLINE int fs_puts(const char* s)
{
    const int fd = 1;
    const char newline = '\n';

    fs_syscall3(FS_SYS_write, fd, (long)s, fs_strlen(s));
    fs_syscall3(FS_SYS_write, fd, (long)&newline, 1);

    return 0;
}

FS_INLINE int fs_put_oct(uint64_t x)
{
    fs_intstr_buf_t buf;
    return fs_puts(fs_uint64_octstr(&buf, x, NULL));
}

FS_INLINE int fs_put_dec(uint64_t x)
{
    fs_intstr_buf_t buf;
    return fs_puts(fs_uint64_decstr(&buf, x, NULL));
}

FS_INLINE int fs_put_hex(uint64_t x)
{
    fs_intstr_buf_t buf;
    return fs_puts(fs_uint64_hexstr(&buf, x, NULL));
}

#endif /* _FREESTANDING_PRINT_H */
