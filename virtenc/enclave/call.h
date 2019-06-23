// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_CALL_H
#define _VE_ENCLAVE_CALL_H

#include <openenclave/bits/types.h>
#include "../common/call.h"

void ve_handle_call_add_thread(uint64_t arg_in);

void ve_handle_call_terminate(void);

void ve_handle_call_ping(int fd, uint64_t arg_in, uint64_t* arg_out);

void ve_handle_call_terminate_thread(void);

void* ve_call_malloc(size_t size);

void* ve_call_calloc(size_t nmemb, size_t size);

void* ve_call_realloc(void* ptr, size_t size);

void* ve_call_memalign(size_t alignment, size_t size);

void ve_call_free(void* ptr);

#endif /* _VE_ENCLAVE_CALL_H */
