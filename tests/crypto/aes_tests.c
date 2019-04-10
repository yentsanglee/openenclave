// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#if defined(OE_BUILD_ENCLAVE)
#include <openenclave/enclave.h>
#endif

#include <openenclave/internal/aes.h>
#include <openenclave/internal/tests.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tests.h"
#include "utils.h"

// Test vector found here:
// https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf
#define NIST_AES_CTR_KEY "2b7e151628aed2a6abf7158809cf4f3c"
#define NIST_AES_CTR_CTR "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"

#define NIST_AES_CTR_PLAIN             \
    "6bc1bee22e409f96e93d7e117393172a" \
    "ae2d8a571e03ac9c9eb76fac45af8e51" \
    "30c81c46a35ce411e5fbc1191a0a52ef" \
    "f69f2445df4f9b17ad2b417be66c3710"

#define NIST_AES_CTR_CIPHER            \
    "874d6191b620e3261bef6864990db6ce" \
    "9806f66b7970fdff8617187bb9fffdff" \
    "5ae4df3edbd5d35e5b4f09020db03eab" \
    "1e031dda2fbe03d1792170a0f3009cee"

// GCM Test vector found here:
// https://csrc.nist.gov/Projects/cryptographic-algorithm-validation-program/CAVP-TESTING-BLOCK-CIPHER-MODES#GCMVS
#define NIST_AES_GCM_KEY "f0305c7b513960533519473976f02beb"
#define NIST_AES_GCM_IV "1a7f6ea0e6c9aa5cf8b78b09"
#define NIST_AES_GCM_PLAIN             \
    "e5fc990c0739e05bd4655871c7401128" \
    "117737a11d520372239ab723f7fde78d" \
    "c4212ac565ee5ee100a014dbb71ea13cdb08eb"

#define NIST_AES_GCM_CIPHER            \
    "30043bcbe2177ab25e4b00a92ee1cd80" \
    "e9daaea0bc0a827fc5fcb84e7b07be63" \
    "95582a5a14e768dde80a20dae0a8b1d8d1d29b"

#define NIST_AES_GCM_AAD               \
    "7e2071cc1c70719143981de543cd28db" \
    "ceb92de0d6021bda4417e7b6417938b1" \
    "26632ecff6e00766e5d0aad3d6f06811"

#define NIST_AES_GCM_TAG "796c41624f6c3cab762380d21ab6130b"

void TestAES_CTR(void)
{
#ifdef OE_BUILD_ENCLAVE
    uint8_t key[16];
    uint8_t counter[16];
    uint8_t plain[64];
    uint8_t cipher[64];
    uint8_t temp[64];

    printf("=== begin %s()\n", __FUNCTION__);

    hex_to_buf(NIST_AES_CTR_KEY, key, sizeof(key));
    hex_to_buf(NIST_AES_CTR_CTR, counter, sizeof(counter));
    hex_to_buf(NIST_AES_CTR_PLAIN, plain, sizeof(plain));
    hex_to_buf(NIST_AES_CTR_CIPHER, cipher, sizeof(cipher));

    oe_aes_ctr_context_t* ctx = NULL;

    oe_aes_ctr_init(&ctx, key, sizeof(key), counter, sizeof(counter), true);
    oe_aes_ctr_crypt(ctx, plain, temp, sizeof(plain));
    oe_aes_ctr_free(ctx);
    OE_TEST(memcmp(cipher, temp, sizeof(temp)) == 0);

    oe_aes_ctr_init(&ctx, key, sizeof(key), counter, sizeof(counter), false);
    oe_aes_ctr_crypt(ctx, cipher, temp, sizeof(cipher));
    oe_aes_ctr_free(ctx);
    OE_TEST(memcmp(plain, temp, sizeof(temp)) == 0);

    printf("=== passed %s()\n", __FUNCTION__);
#endif
}

void TestAES_GCM(void)
{
#ifdef OE_BUILD_ENCLAVE
    uint8_t key[16];
    uint8_t iv[12];
    uint8_t plain[51];
    uint8_t cipher[51];
    uint8_t aad[48];
    uint8_t tag[16];
    uint8_t temp[51];
    oe_aes_gcm_tag_t temp_tag;

    printf("=== begin %s()\n", __FUNCTION__);

    hex_to_buf(NIST_AES_GCM_KEY, key, sizeof(key));
    hex_to_buf(NIST_AES_GCM_IV, iv, sizeof(iv));
    hex_to_buf(NIST_AES_GCM_PLAIN, plain, sizeof(plain));
    hex_to_buf(NIST_AES_GCM_CIPHER, cipher, sizeof(cipher));
    hex_to_buf(NIST_AES_GCM_AAD, aad, sizeof(aad));
    hex_to_buf(NIST_AES_GCM_TAG, tag, sizeof(tag));

    oe_aes_gcm_context_t* ctx = NULL;
    oe_aes_gcm_init(
        &ctx, key, sizeof(key), iv, sizeof(iv), aad, sizeof(aad), true);
    for (size_t i = 0; i < sizeof(plain); i += 16)
    {
        size_t len = (sizeof(plain) - i < 16) ? sizeof(plain) - i : 16;
        oe_aes_gcm_update(ctx, plain + i, temp + i, len);
    }
    oe_aes_gcm_final(ctx, &temp_tag);
    oe_aes_gcm_free(ctx);
    OE_TEST(memcmp(cipher, temp, sizeof(temp)) == 0);
    OE_TEST(memcmp(tag, temp_tag.buf, sizeof(tag)) == 0);

    memset(temp_tag.buf, 0, sizeof(temp_tag.buf));
    oe_aes_gcm_init(
        &ctx, key, sizeof(key), iv, sizeof(iv), aad, sizeof(aad), false);
    for (size_t i = 0; i < sizeof(cipher); i += 16)
    {
        size_t len = (sizeof(cipher) - i < 16) ? sizeof(cipher) - i : 16;
        oe_aes_gcm_update(ctx, cipher + i, temp + i, len);
    }
    oe_aes_gcm_final(ctx, &temp_tag);
    oe_aes_gcm_free(ctx);
    OE_TEST(memcmp(plain, temp, sizeof(temp)) == 0);
    OE_TEST(memcmp(tag, temp_tag.buf, sizeof(tag)) == 0);

    printf("=== passed %s()\n", __FUNCTION__);
#endif
}
