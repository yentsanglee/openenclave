/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


//#include "sgx_tcrypto.h"
//#include "ippcp.h"
#include <openenclave/enclave.h>
#include <sgx2oe.h>
#include <mbedtls/gcm.h>
#include "stdlib.h"
#include "string.h"

/* Rijndael AES-GCM
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined sgx_error.h
*   Inputs: sgx_aes_gcm_128bit_key_t *p_key - Pointer to key used in encryption/decryption operation
*           uint8_t *p_src - Pointer to input stream to be encrypted/decrypted
*           uint32_t src_len - Length of input stream to be encrypted/decrypted
*           uint8_t *p_iv - Pointer to initialization vector to use
*           uint32_t iv_len - Length of initialization vector
*           uint8_t *p_aad - Pointer to input stream of additional authentication data
*           uint32_t aad_len - Length of additional authentication data stream
*           sgx_aes_gcm_128bit_tag_t *p_in_mac - Pointer to expected MAC in decryption process
*   Output: uint8_t *p_dst - Pointer to cipher text. Size of buffer should be >= src_len.
*           sgx_aes_gcm_128bit_tag_t *p_out_mac - Pointer to MAC generated from encryption process
* NOTE: Wrapper is responsible for confirming decryption tag matches encryption tag */
oe_result_t oe_rijndael128GCM_encrypt(const oe_aes_gcm_128bit_key_t *p_key, const uint8_t *p_src, uint32_t src_len,
                                        uint8_t *p_dst, const uint8_t *p_iv, uint32_t iv_len, const uint8_t *p_aad, uint32_t aad_len,
                                        oe_aes_gcm_128bit_tag_t *p_out_mac)
{
    if ((p_key == NULL) || ((src_len > 0) && (p_dst == NULL)) || ((src_len > 0) && (p_src == NULL))
        || (p_out_mac == NULL) || (iv_len != OE_AESGCM_IV_SIZE) || ((aad_len > 0) && (p_aad == NULL))
        || (p_iv == NULL) || ((p_src == NULL) && (p_aad == NULL)))
    {
        return OE_FAILURE;
    }

    mbedtls_gcm_context gcm_context;
    mbedtls_gcm_init(&gcm_context);
    if (mbedtls_gcm_setkey(&gcm_context,MBEDTLS_CIPHER_ID_AES, (const unsigned char *) p_key, 128) != 0)
    {
        return OE_FAILURE;
    }

    if (mbedtls_gcm_crypt_and_tag(&gcm_context, MBEDTLS_GCM_ENCRYPT, src_len, p_iv, iv_len, p_aad, aad_len, p_src, p_dst,sizeof(oe_aes_gcm_128bit_tag_t), (unsigned char*) p_out_mac ) != 0)
    {
        return OE_FAILURE;
    }
    mbedtls_gcm_free(&gcm_context);

    return OE_OK;
}

oe_result_t oe_rijndael128GCM_decrypt(const oe_aes_gcm_128bit_key_t *p_key, const uint8_t *p_src,
                                        uint32_t src_len, uint8_t *p_dst, const uint8_t *p_iv, uint32_t iv_len,
                                        const uint8_t *p_aad, uint32_t aad_len, const oe_aes_gcm_128bit_tag_t *p_in_mac)
{
    mbedtls_gcm_context gcm_context;
    mbedtls_gcm_init(&gcm_context);
    if (mbedtls_gcm_setkey(&gcm_context,MBEDTLS_CIPHER_ID_AES, (const unsigned char *) p_key, 128) != 0)
    {
        return OE_FAILURE;
    }

    if (mbedtls_gcm_auth_decrypt(&gcm_context, src_len, p_iv, iv_len, p_aad, aad_len, (const unsigned char*) p_in_mac, 16, p_src, p_dst) != 0)
    {
        return OE_FAILURE;
    }

    mbedtls_gcm_free(&gcm_context);

    return OE_OK;

}

