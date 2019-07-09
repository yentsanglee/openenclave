// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_COMMON_H
#define _VE_ENCLAVE_COMMON_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/corelibc/stdarg.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/internal/defs.h>

#if defined(__GNUC__)
#define VE_WEAK __attribute__((__weak__))
#endif

#endif /* _VE_ENCLAVE_COMMON_H */
