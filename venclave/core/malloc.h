// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_MALLOC_H
#define _VE_ENCLAVE_MALLOC_H

#include <openenclave/corelibc/stdlib.h>
#include "common.h"

void* oe_malloc(size_t size);

void oe_free(void* ptr);

void* oe_calloc(size_t nmemb, size_t size);

void* oe_realloc(void* ptr, size_t size);

void* oe_memalign(size_t alignment, size_t size);

int oe_posix_memalign(void** memptr, size_t alignment, size_t size);

#endif /* _VE_ENCLAVE_MALLOC_H */
