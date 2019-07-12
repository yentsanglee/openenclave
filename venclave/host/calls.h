// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_CALLS_H
#define _VE_HOST_CALLS_H

#include "../common/call.h"

int ve_call_handler(void* handler_arg, int fd, ve_call_buf_t* buf);

int ve_handle_call_host_function(void* handler_arg, int fd, ve_call_buf_t* buf);

int ve_handle_ocall(void* handler_arg, int fd, ve_call_buf_t* buf);

#endif /* _VE_HOST_CALLS_H */
