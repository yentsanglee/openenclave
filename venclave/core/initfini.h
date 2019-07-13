// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_INITFINI_H
#define _VE_ENCLAVE_INITFINI_H

#include "common.h"

void ve_call_init_functions(void);

void ve_call_fini_functions(void);

#endif /* _VE_ENCLAVE_INITFINI_H */
