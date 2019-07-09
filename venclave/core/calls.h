// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_CALLS_H
#define _VE_ENCLAVE_CALLS_H

#include "../common/call.h"
#include "common.h"

int ve_handle_init_enclave(int fd, ve_call_buf_t* buf, int* exit_status);

int ve_handle_post_init(int fd, ve_call_buf_t* buf, int* exit_status);

int ve_handle_get_settings(int fd, ve_call_buf_t* buf, int* exit_status);

int ve_handle_call_add_thread(int fd, ve_call_buf_t* buf, int* exit_status);

int ve_handle_call_terminate(int fd, ve_call_buf_t* buf, int* exit_status);

int ve_handle_call_terminate_thread(
    int fd,
    ve_call_buf_t* buf,
    int* exit_status);

int ve_handle_call_enclave_function(
    int fd,
    ve_call_buf_t* buf,
    int* exit_status);

int ve_handle_calls(int fd);

void* ve_call_malloc(int fd, size_t size);

void* ve_call_calloc(int fd, size_t nmemb, size_t size);

void* ve_call_realloc(int fd, void* ptr, size_t size);

void* ve_call_memalign(int fd, size_t alignment, size_t size);

int ve_call_free(int fd, void* ptr);

#endif /* _VE_ENCLAVE_CALLS_H */
