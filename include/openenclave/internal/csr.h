// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/ec.h>

oe_result_t oe_csr_from_ec_key(
    oe_ec_private_key_t* private_key_,
    oe_ec_public_key_t* public_key_,
    const char* name,
    uint8_t** csr,
    size_t* csr_size);
