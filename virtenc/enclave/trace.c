// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "trace.h"
#include "../common/lock.h"
#include "put.h"
#include "string.h"

static ve_lock_t _lock;

void __ve_trace(const char* file, unsigned int line)
{
    ve_intstr_buf_t buf;

    ve_lock(&_lock);
    ve_put("enclave: trace: ");
    ve_put(file);
    ve_put("(");
    ve_put(ve_uint64_decstr(&buf, line, NULL));
    ve_put(")");
    ve_put_n();
    ve_unlock(&_lock);
}
