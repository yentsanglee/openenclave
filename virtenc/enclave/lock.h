// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_LOCK_H
#define _VE_ENCLAVE_LOCK_H

#include "common.h"

typedef volatile int ve_lock_t;

void ve_lock(ve_lock_t* lock);

void ve_unlock(ve_lock_t* lock);

#endif /* _VE_ENCLAVE_LOCK_H */
