// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_ENCLAVE_H
#define _VE_HOST_ENCLAVE_H

#include <stddef.h>
#include <stdint.h>

typedef struct _ve_enclave ve_enclave_t;

int ve_create_enclave(const char* path, ve_enclave_t** enclave_out);

int ve_terminate_enclave(ve_enclave_t* enclave);

#endif /* _VE_HOST_ENCLAVE_H */
