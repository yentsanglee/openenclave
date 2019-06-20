// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "put.h"
#include <openenclave/internal/syscall/unistd.h>
#include "../common/lock.h"
#include "exit.h"
#include "string.h"
#include "syscall.h"

static ve_lock_t _lock;

void ve_put(const char* s)
{
    ve_syscall3(OE_SYS_write, OE_STDOUT_FILENO, (long)s, ve_strlen(s));
}

void ve_put_nl(void)
{
    const char nl = '\n';
    ve_syscall3(OE_SYS_write, OE_STDOUT_FILENO, (long)&nl, 1);
}

void ve_puts(const char* s)
{
    ve_lock(&_lock);
    ve_put(s);
    ve_put_nl();
    ve_unlock(&_lock);
}

void ve_put_oct(const char* s, uint64_t x)
{
    ve_lock(&_lock);
    ve_intstr_buf_t buf;
    ve_put(s);
    ve_put(ve_uint64_octstr(&buf, x, NULL));
    ve_put_nl();
    ve_unlock(&_lock);
}

void ve_put_uint(const char* s, uint64_t x)
{
    ve_lock(&_lock);
    ve_intstr_buf_t buf;
    ve_put(s);
    ve_put(ve_uint64_decstr(&buf, x, NULL));
    ve_put_nl();
    ve_unlock(&_lock);
}

void ve_put_int(const char* s, int64_t x)
{
    ve_lock(&_lock);
    ve_intstr_buf_t buf;
    ve_put(s);
    ve_put(ve_int64_decstr(&buf, x, NULL));
    ve_put_nl();
    ve_unlock(&_lock);
}

void ve_put_hex(const char* s, uint64_t x)
{
    ve_lock(&_lock);
    ve_intstr_buf_t buf;
    ve_put(s);
    ve_put(ve_uint64_hexstr(&buf, x, NULL));
    ve_put_nl();
    ve_unlock(&_lock);
}

void ve_put_err(const char* s)
{
    ve_lock(&_lock);
    const char nl = '\n';
    ve_syscall3(OE_SYS_write, OE_STDERR_FILENO, (long)s, ve_strlen(s));
    ve_syscall3(OE_SYS_write, OE_STDERR_FILENO, (long)&nl, 1);
    ve_unlock(&_lock);
    ve_exit(1);
}
