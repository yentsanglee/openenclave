// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_MSG_H
#define _VE_ENCLAVE_MSG_H

#include "../common/msg.h"

int ve_handle_init(void);

int ve_handle_calls(int fd);

#endif /* _VE_ENCLAVE_MSG_H */
