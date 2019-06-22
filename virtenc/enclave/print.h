// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_PRINT_H
#define _VE_PRINT_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

void ve_put(const char* s);

void ve_puts(const char* s);

OE_PRINTF_FORMAT(1, 2)
void ve_print(const char* format, ...);

#endif /* _VE_PRINT_H */
