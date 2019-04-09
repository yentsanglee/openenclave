// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#if defined(OE_BUILD_ENCLAVE)
#include <openenclave/enclave.h>
#endif

#include <openenclave/internal/cmac.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "hash.h"
#include "tests.h"

// Test compution of SHA256-HMAC over an ASCII string alphabet.
void TestCMAC(void)
{
#ifdef OE_BUILD_ENCLAVE
    printf("=== begin %s()\n", __FUNCTION__);

    oe_aes_cmac_t cmac;
    oe_aes_cmac_context_t* ctx;
    oe_aes_cmac_init(&ctx, ALPHABET_KEY, ALPHABET_KEY_SIZE);
    oe_aes_cmac_update(ctx, (const uint8_t*)ALPHABET, strlen(ALPHABET));
    oe_aes_cmac_final(ctx, &cmac);
    oe_aes_cmac_free(ctx);

    oe_aes_cmac_t cmac2;
    oe_aes_cmac_sign(
        ALPHABET_KEY,
        ALPHABET_KEY_SIZE,
        (const uint8_t*)ALPHABET,
        strlen(ALPHABET),
        &cmac2);

    OE_TEST(memcmp(&cmac, &ALPHABET_CMAC, sizeof(ALPHABET_CMAC)) == 0);
    OE_TEST(memcmp(&cmac2, &ALPHABET_CMAC, sizeof(ALPHABET_CMAC)) == 0);

    printf("=== passed %s()\n", __FUNCTION__);
#endif
}
