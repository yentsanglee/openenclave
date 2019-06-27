// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_CALL_H
#define _VE_ENCLAVE_CALL_H

#include <openenclave/bits/types.h>
#include "../common/call.h"

int ve_handle_call_ping(int fd, ve_call_buf_t* buf);

int ve_handle_call_add_thread(int fd, ve_call_buf_t* buf);

int ve_handle_call_terminate(int fd, ve_call_buf_t* buf);

int ve_handle_call_terminate_thread(int fd, ve_call_buf_t* buf);

int ve_handle_call_ecall(int fd, ve_call_buf_t* buf);

int ve_handle_get_settings(int fd, ve_call_buf_t* buf);

void* ve_call_malloc(int fd, size_t size);

void* ve_call_calloc(int fd, size_t nmemb, size_t size);

void* ve_call_realloc(int fd, void* ptr, size_t size);

void* ve_call_memalign(int fd, size_t alignment, size_t size);

int ve_call_free(int fd, void* ptr);

#endif /* _VE_ENCLAVE_CALL_H */
