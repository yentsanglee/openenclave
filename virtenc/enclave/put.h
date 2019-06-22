// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_PUT_H
#define _VE_PUT_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

void ve_put(const char* s);

void ve_put_n(void);

void ve_put_s(const char* s, const char* x);

void ve_put_o(const char* s, uint64_t x);

void ve_put_u(const char* s, uint64_t x);

void ve_put_i(const char* s, int64_t x);

void ve_put_x(const char* s, uint64_t x);

void ve_put_e(const char* s);

#endif /* _VE_PUT_H */
