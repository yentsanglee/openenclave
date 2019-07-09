// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_PRINT_H
#define _VE_ENCLAVE_PRINT_H

#include "common.h"
#include "lock.h"

void ve_put(const char* s);

void ve_puts(const char* s);

void ve_putc(char c);

OE_PRINTF_FORMAT(1, 2)
void ve_printf(const char* format, ...);

void ve_vprintf(const char* format, oe_va_list ap);

extern ve_lock_t __ve_print_lock;

#endif /* _VE_ENCLAVE_PRINT_H */
