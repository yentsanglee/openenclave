// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/crypto/sha.h>
#include <openenclave/internal/ec.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "vcrypto_t.h"

static const OE_SHA256 ALPHABET_HASH = {{
    0x71, 0xc4, 0x80, 0xdf, 0x93, 0xd6, 0xae, 0x2f, 0x1e, 0xfa, 0xd1,
    0x44, 0x7c, 0x66, 0xc9, 0x52, 0x5e, 0x31, 0x62, 0x18, 0xcf, 0x51,
    0xfc, 0x8d, 0x9e, 0xd8, 0x32, 0xf2, 0xda, 0xf1, 0x8b, 0x73,
}};

static void _test_generate_common(
    const oe_ec_private_key_t* private_key,
    const oe_ec_public_key_t* public_key)
{
    oe_result_t r;
    uint8_t* signature = NULL;
    size_t signature_size = 0;

    r = oe_ec_private_key_sign(
        private_key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        signature,
        &signature_size);
    OE_TEST(r == OE_BUFFER_TOO_SMALL);

    OE_TEST(signature = (uint8_t*)malloc(signature_size));

    r = oe_ec_private_key_sign(
        private_key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        signature,
        &signature_size);
    OE_TEST(r == OE_OK);

    r = oe_ec_public_key_verify(
        public_key,
        OE_HASH_TYPE_SHA256,
        &ALPHABET_HASH,
        sizeof(ALPHABET_HASH),
        signature,
        signature_size);
    OE_TEST(r == OE_OK);

    free(signature);
}

static void _test_sha256(void)
{
    const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";
    OE_SHA256 hash = {0};
    oe_sha256_context_t ctx = {0};

    printf("=== begin %s()\n", __FUNCTION__);

    oe_sha256_init(&ctx);
    oe_sha256_update(&ctx, ALPHABET, strlen(ALPHABET));
    oe_sha256_final(&ctx, &hash);
    OE_TEST(memcmp(&hash, &ALPHABET_HASH, sizeof(OE_SHA256)) == 0);

    printf("=== passed %s()\n", __FUNCTION__);
}

static void _test_generate(void)
{
    oe_result_t r;
    oe_ec_private_key_t private_key = {0};
    oe_ec_public_key_t public_key = {0};

    printf("=== begin %s()\n", __FUNCTION__);

    r = oe_ec_generate_key_pair(
        OE_EC_TYPE_SECP256R1, &private_key, &public_key);
    OE_TEST(r == OE_OK);

    _test_generate_common(&private_key, &public_key);

    oe_ec_private_key_free(&private_key);
    oe_ec_public_key_free(&public_key);

    printf("=== passed %s()\n", __FUNCTION__);
}

void test_vcrypto(void)
{
    _test_sha256();
    _test_generate();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    8);   /* TCSCount */
