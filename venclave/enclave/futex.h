// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_FUTEX_H
#define _VE_ENCLAVE_FUTEX_H

#include "common.h"

int ve_futex_wait(volatile int* uaddr, int val, int priv);

int ve_futex_wake(volatile int* uaddr, int val, int priv);

#endif /* _VE_ENCLAVE_FUTEX_H */
