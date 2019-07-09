// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_HOSTHEAP_H
#define _VE_ENCLAVE_HOSTHEAP_H

#include "common.h"

int ve_host_heap_attach(int shmid, void* shmaddr, size_t shmsize);

void* ve_host_heap_get_start(void);

void* ve_host_heap_get_end(void);

size_t ve_host_heap_get_size(void);

#endif /* _VE_ENCLAVE_HOSTHEAP_H */
