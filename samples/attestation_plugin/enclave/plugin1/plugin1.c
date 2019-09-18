// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <mbedtls/sha256.h>
#include <openenclave/bits/defs.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/enclave.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plugin1.h"

// This plugin emulates the functionality of `oe_get_report` and
// `oe_verify_report`  for remote attestation reports through
// the plugin interface.

static int oe_on_register(
    oe_quote_customization_plugin_context_t* plugin_context,
    const void* config_data,
    size_t config_data_size)
{
    OE_UNUSED(plugin_context);
    OE_UNUSED(config_data);
    OE_UNUSED(config_data_size);
    return 0;
}

static int oe_on_unregister(
    oe_quote_customization_plugin_context_t* plugin_context)
{
    OE_UNUSED(plugin_context);
    return 0;
}

static int _calc_sha256(
    const uint8_t* user_data,
    size_t user_data_size,
    uint8_t* output)
{
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    int ret = -1;

    if (mbedtls_sha256_starts_ret(&ctx, 0) != 0)
        goto done;

    if (mbedtls_sha256_update_ret(&ctx, user_data, user_data_size) != 0)
        goto done;

    if (mbedtls_sha256_finish_ret(&ctx, output) != 0)
        goto done;

    ret = 0;

done:
    mbedtls_sha256_free(&ctx);
    return ret;
}

static int oe_get_evidence(
    oe_quote_customization_plugin_context_t* plugin_context,
    const uint8_t* user_data,
    size_t user_data_size,
    uint8_t** evidence_buffer,
    size_t* evidence_buffer_size)
{
    // Need to hash the user data first.
    uint8_t* report_data = NULL;
    size_t report_data_size = 0;
    uint8_t hash[32];
    int ret = -1;
    oe_result_t oe_ret;
    uint8_t* report_buffer = NULL;
    size_t report_buffer_size = 0;
    size_t total_evidence_size = 0;

    if (user_data)
    {
        if (_calc_sha256(user_data, user_data_size, hash) != 0)
            goto done;

        report_data = hash;
        report_data_size = sizeof(hash);
    }

    // Technically, this will wrap the sgx report with the OE header,
    // which will get wrapped again in the caller of this function,
    // but that's fine.
    oe_ret = oe_get_report(
        OE_REPORT_FLAGS_REMOTE_ATTESTATION,
        report_data,
        report_data_size,
        NULL,
        0,
        &report_buffer,
        &report_buffer_size);

    if (oe_ret != OE_OK)
        goto done;

    if (!user_data)
    {
        ret = 0;
        goto done;
    }

    // Now, we need to append the report data to the end of the report.
    oe_ret = oe_safe_add_sizet(
        report_data_size, report_buffer_size, &total_evidence_size);
    if (oe_ret != OE_OK)
        goto done;

    *evidence_buffer = (uint8_t*)malloc(total_evidence_size);
    if (*evidence_buffer == NULL)
        goto done;

    memcpy(*evidence_buffer, report_buffer, report_buffer_size);
    memcpy(
        *evidence_buffer + report_buffer_size, report_data, report_data_size);
    *evidence_buffer_size = total_evidence_size;

    ret = 0;

done:
    return ret;
}

static int oe_free_evidence(
    oe_quote_customization_plugin_context_t* plugin_context,
    uint8_t* evidence_buffer)
{
    oe_free_report(evidence_buffer);
    return 0;
}

static int oe_verify_evidence(
    oe_quote_customization_plugin_context_t* plugin_context,
    const uint8_t* evidence_buffer,
    size_t evidence_buffer_size,
    oe_claim_element_t** claims,
    size_t* claim_count,
    uint8_t** user_data,
    size_t* user_data_size)
{
    return 0;
}

oe_quote_customization_plugin_context_t oe_plugin_context = {
    .evidence_format_uuid = UUID_INIT(
        0x6EBB65E5,
        0xF657,
        0x48B1,
        0x94,
        0xDF,
        0x0E,
        0xC0,
        0xB6,
        0x71,
        0xDA,
        0x26),
    .on_register = oe_on_register,
    .on_unregister = oe_on_unregister,
    .get_evidence = oe_get_evidence,
    .free_evidence = oe_free_evidence,
    .verify_evidence = oe_verify_evidence};

oe_quote_customization_plugin_context_t* create_oe_plugin()
{
    return &oe_plugin_context;
}
