// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "settings.h"

ve_enclave_settings_t __ve_enclave_settings = {
    .num_heap_pages = 1024,
    .num_stack_pages = 256,
    .num_tcs = 2,
};
