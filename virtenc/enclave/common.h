// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_COMMON_H
#define _VE_ENCLAVE_COMMON_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

#if defined(__GNUC__)
#define ve_va_list __builtin_va_list
#define ve_va_start __builtin_va_start
#define ve_va_arg __builtin_va_arg
#define ve_va_end __builtin_va_end
#define ve_va_copy __builtin_va_copy
#endif

#endif /* _VE_ENCLAVE_COMMON_H */
