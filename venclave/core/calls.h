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

#endif /* _VE_ENCLAVE_CALLS_H */
