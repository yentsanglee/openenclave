// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_PUT_H
#define _VE_PUT_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

void ve_put(const char* s);

void ve_puts(const char* s);

void ve_put_nl(void);

void ve_put_str(const char* s, const char* x);

void ve_put_oct(const char* s, uint64_t x);

void ve_put_uint(const char* s, uint64_t x);

void ve_put_int(const char* s, int64_t x);

void ve_put_hex(const char* s, uint64_t x);

void ve_put_err(const char* s);

void __ve_trace(const char* file, unsigned int line);

#endif /* _VE_PUT_H */
