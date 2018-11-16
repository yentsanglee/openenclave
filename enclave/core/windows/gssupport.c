// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/utils.h>

/* Declare security cookie here to make sure it's present. */
__declspec(selectany) uint64_t __security_cookie;
__declspec(selectany) uint64_t __security_cookie_complement;

uint32_t oe_cookie_initialized;