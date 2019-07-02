// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_PANIC_H
#define _VE_ENCLAVE_PANIC_H

#define ve_panic(EXPR) __ve_panic(#EXPR, __FILE__, __LINE__, __FUNCTION__)

void __ve_panic(
    const char* expr,
    const char* file,
    int line,
    const char* function);

#endif /* _VE_ENCLAVE_PANIC_H */
