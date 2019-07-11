// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_SBRK_H
#define _VE_ENCLAVE_SBRK_H

#include "common.h"

/* The break value after the first call to ve_sbrk(). */
extern void* __ve_initial_brk_value;

/* Change the location of the program break value (invokes brk system call). */
void* ve_sbrk(intptr_t increment);

#endif /* _VE_ENCLAVE_SBRK_H */
