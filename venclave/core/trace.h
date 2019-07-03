// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_TRACE_H
#define _VE_ENCLAVE_TRACE_H

#include "print.h"

#define VE_TRACE ve_print("encl: trace: %s(%u)\n", __FILE__, __LINE__)

#if defined(VE_DO_TRACE)
#define VE_T(EXPR) EXPR
#else
#define VE_T(EXPR)
#endif

#endif /* _VE_ENCLAVE_TRACE_H */
