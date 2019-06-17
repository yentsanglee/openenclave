// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_PRINT_H
#define _VE_PRINT_H

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "string.h"
#include "syscall.h"

OE_INLINE void ve_print_str(const char* s)
{
    ve_syscall(OE_SYS_write, OE_STDERR_FILENO, (long)s, ve_strlen(s));
}

OE_INLINE void ve_print_nl(void)
{
    const char nl = '\n';
    ve_syscall(OE_SYS_write, OE_STDERR_FILENO, (long)&nl, 1);
}

OE_INLINE void ve_print_oct(uint64_t x)
{
    ve_intstr_buf_t buf;
    ve_print_str(ve_uint64_octstr(&buf, x, NULL));
}

OE_INLINE void ve_print_uint(uint64_t x)
{
    ve_intstr_buf_t buf;
    ve_print_str(ve_uint64_decstr(&buf, x, NULL));
}

OE_INLINE void ve_print_int(int64_t x)
{
    ve_intstr_buf_t buf;
    ve_print_str(ve_int64_decstr(&buf, x, NULL));
}

OE_INLINE void ve_print_hex(uint64_t x)
{
    ve_intstr_buf_t buf;
    ve_print_str(ve_uint64_hexstr(&buf, x, NULL));
}

#endif /* _VE_PRINT_H */
