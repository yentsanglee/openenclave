// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_SBRK_H
#define _VE_SBRK_H

#include "common.h"

/* Change the location of the program break value (invokes brk system call). */
void* ve_sbrk(intptr_t increment);

#endif /* _VE_SBRK_H */
