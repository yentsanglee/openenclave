// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_AES_H
#define _OE_AES_H

#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

typedef struct _oe_aes_gcm_context oe_aes_gcm_context_t;

typedef struct _oe_aes_gcm_tag
{
    uint8_t buf[16];
} oe_aes_gcm_tag_t;

typedef struct _oe_aes_ctr_context oe_aes_ctr_context_t;

/******* AES-GCM Methods ********/
oe_result_t oe_aes_gcm_init(
    oe_aes_gcm_context_t** context,
    const uint8_t* key,
    size_t key_size,
    const uint8_t* iv,
    size_t iv_size,
    const uint8_t* aad,
    size_t add_size,
    bool encrypt);

oe_result_t oe_aes_gcm_update(
    oe_aes_gcm_context_t* context,
    const uint8_t* input,
    uint8_t* output,
    size_t size);

oe_result_t oe_aes_gcm_final(
    oe_aes_gcm_context_t* context,
    oe_aes_gcm_tag_t* tag);

oe_result_t oe_aes_gcm_free(oe_aes_gcm_context_t* context);

/******* AES-CTR Methods ********/
oe_result_t oe_aes_ctr_init(
    oe_aes_ctr_context_t** context,
    const uint8_t* key,
    size_t key_size,
    const uint8_t* iv,
    size_t iv_size,
    bool encrypt);

oe_result_t oe_aes_ctr_crypt(
    oe_aes_ctr_context_t* context,
    const uint8_t* input,
    uint8_t* output,
    size_t size);

oe_result_t oe_aes_ctr_free(oe_aes_ctr_context_t* context);

OE_EXTERNC_END

#endif /* _OE_AES_H */
