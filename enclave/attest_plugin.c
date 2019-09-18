// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/bits/safecrt.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/cert.h>
#include <openenclave/internal/crypto/sha.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/report.h>
#include <openenclave/internal/sgxtypes.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/utils.h>
#include <stdio.h>
#include <string.h>

#include "../common/common.h"

struct plugin_list_node_t
{
    oe_quote_customization_plugin_context_t* plugin_context;
    struct plugin_list_node_t* next;
};

static oe_mutex_t g_plugin_list_mutex = OE_MUTEX_INITIALIZER;
struct plugin_list_node_t* g_plugins = NULL;

static void _dump_attestation_plugin_list()
{
    struct plugin_list_node_t* cur = NULL;

    OE_TRACE_INFO(
        "Calling oe_register_attestation_plugin: evidence_format_uuid list\n");

    oe_mutex_lock(&g_plugin_list_mutex);
    cur = g_plugins;
    while (cur)
    {
        for (int i = 0; i < UUID_SIZE; i++)
        {
            OE_TRACE_INFO(
                "0x%0x\n", cur->plugin_context->evidence_format_uuid.b[i]);
        }
        cur = cur->next;
    }
    oe_mutex_unlock(&g_plugin_list_mutex);
}

static struct plugin_list_node_t* _find_plugin(
    const uuid_t* target_evidence_format_uuid,
    struct plugin_list_node_t** prev)
{
    struct plugin_list_node_t* ret = NULL;
    struct plugin_list_node_t* cur = NULL;

    if (prev)
        *prev = NULL;

    // Find a plugin for attestation type.
    oe_mutex_lock(&g_plugin_list_mutex);
    cur = g_plugins;
    while (cur)
    {
        if (memcmp(
                &cur->plugin_context->evidence_format_uuid,
                target_evidence_format_uuid,
                sizeof(uuid_t)) == 0)
        {
            ret = cur;
            break;
        }
        if (prev)
            *prev = cur;
        cur = cur->next;
    }
    oe_mutex_unlock(&g_plugin_list_mutex);
    return ret;
}

oe_result_t oe_register_attestation_plugin(
    oe_quote_customization_plugin_context_t* plugin,
    const void* config_data,
    size_t config_data_size)
{
    oe_result_t result = OE_UNEXPECTED;
    struct plugin_list_node_t* plugin_node = NULL;

    OE_TRACE_INFO("Calling oe_register_attestation_plugin");

    plugin_node = _find_plugin(&plugin->evidence_format_uuid, NULL);
    if (plugin_node)
    {
        OE_TRACE_ERROR(
            "Calling oe_register_attestation_plugin failed: "
            "evidence_format_uuid[%s] already existed",
            plugin->evidence_format_uuid);
        plugin_node = NULL;
        OE_RAISE(OE_ALREADY_EXISTS);
    }

    plugin_node = (struct plugin_list_node_t*)oe_malloc(sizeof(*plugin_node));
    if (plugin_node == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    // Run the register function for the plugin.
    if (plugin->on_register(plugin, config_data, config_data_size) != 0)
    {
        oe_free(plugin_node);
        OE_RAISE(OE_FAILURE);
    }
    plugin_node->plugin_context = plugin;

    // Add to the plugin list.
    oe_mutex_lock(&g_plugin_list_mutex);
    plugin_node->next = g_plugins;
    g_plugins = plugin_node;
    oe_mutex_unlock(&g_plugin_list_mutex);

    _dump_attestation_plugin_list();
    result = OE_OK;

done:
    return result;
}

oe_result_t oe_unregister_attestation_plugin(
    oe_quote_customization_plugin_context_t* plugin)
{
    oe_result_t result = OE_UNEXPECTED;
    struct plugin_list_node_t* prev = NULL;
    struct plugin_list_node_t* cur = g_plugins;

    OE_TRACE_INFO("Calling oe_unregister_attestation_plugin 1");

    // Find the guid and remove it.
    cur = _find_plugin(&plugin->evidence_format_uuid, &prev);
    if (cur == NULL)
    {
        OE_TRACE_ERROR(
            "Calling oe_unregister_attestation_plugin failed: "
            "evidence_format_uuid[%s] was not registered before",
            plugin->evidence_format_uuid);
        OE_RAISE(OE_NOT_FOUND);
    }

    oe_mutex_lock(&g_plugin_list_mutex);
    if (prev != NULL)
        prev->next = cur->next;
    else
        g_plugins = NULL;
    oe_mutex_unlock(&g_plugin_list_mutex);

    // Run the unregister hook for the plugin.
    if (cur->plugin_context->on_unregister(cur->plugin_context) != 0)
        OE_RAISE(OE_FAILURE);

    _dump_attestation_plugin_list();
    result = OE_OK;

done:
    oe_free(cur);
    return result;
}

oe_result_t oe_get_attestation_evidence(
    const uuid_t* evidence_format_uuid,
    const uint8_t* user_data,
    size_t user_data_size,
    uint8_t** evidence_buffer,
    size_t* evidence_buffer_size)
{
    oe_result_t result = OE_UNEXPECTED;
    struct plugin_list_node_t* plugin_node = NULL;
    oe_quote_customization_plugin_context_t* plugin_context = NULL;
    int ret;
    uint8_t* plugin_evidence = NULL;
    size_t plugin_evidence_size = 0;
    uint8_t* total_evidence_buf = NULL;
    size_t total_evidence_size = 0;
    oe_evidence_header_t* header;

    // Find a plugin for attestation type and run its get_evidence.
    plugin_node = _find_plugin(evidence_format_uuid, NULL);
    if (plugin_node == NULL)
        OE_RAISE(OE_NOT_FOUND);

    plugin_context = plugin_node->plugin_context;
    ret = plugin_context->get_evidence(
        plugin_context,
        user_data,
        user_data_size,
        &plugin_evidence,
        &plugin_evidence_size);
    if (ret != 0)
    {
        OE_TRACE_ERROR("get_evidence failed with ret = %d", ret);
        OE_RAISE(OE_FAILURE);
    }

    // Wrap the OE header around it.
    OE_CHECK(oe_safe_add_sizet(
        sizeof(oe_evidence_header_t),
        plugin_evidence_size,
        &total_evidence_size));

    total_evidence_buf = (uint8_t*)oe_malloc(total_evidence_size);
    if (total_evidence_buf == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    // TODO(akagup): Fix oe report header to only have 1 size parameter.
    header = (oe_evidence_header_t*)total_evidence_buf;
    header->version = OE_REPORT_HEADER_VERSION;
    header->evidence_format_uuid = *evidence_format_uuid;
    header->tee_evidence_type = OE_TEE_TYPE_CUSTOM;
    header->tee_evidence_size = 0;
    header->custom_evidence_size = (uint32_t)plugin_evidence_size;
    memcpy(
        total_evidence_buf + sizeof(oe_evidence_header_t),
        plugin_evidence,
        plugin_evidence_size);

    *evidence_buffer = total_evidence_buf;
    *evidence_buffer_size = total_evidence_size;
    total_evidence_buf = NULL;
    result = OE_OK;

done:
    if (plugin_context && plugin_evidence)
        plugin_context->free_evidence(plugin_context, plugin_evidence);
    oe_free(total_evidence_buf);
    return result;
}

void oe_free_attestation_evidence(uint8_t* evidence_buffer)
{
    oe_free(evidence_buffer);
}

oe_result_t oe_verify_attestation_evidence(
    const uint8_t* evidence_buffer,
    size_t evidence_buffer_size,
    oe_claim_element_t** claims,
    size_t* claims_count,
    uint8_t** user_data,
    size_t* user_data_size)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_evidence_header_t* header = (oe_evidence_header_t*)evidence_buffer;
    struct plugin_list_node_t* plugin_node;
    int ret;

    if (!evidence_buffer || evidence_buffer_size < sizeof(*header))
        OE_RAISE(OE_INVALID_PARAMETER);

    plugin_node = _find_plugin(&header->evidence_format_uuid, NULL);
    if (plugin_node == NULL)
        OE_RAISE(OE_NOT_FOUND);

    ret = plugin_node->plugin_context->verify_evidence(
        plugin_node->plugin_context,
        evidence_buffer + sizeof(*header),
        header->custom_evidence_size,
        claims,
        claims_count,
        user_data,
        user_data_size);

    if (ret != 0)
    {
        OE_TRACE_ERROR("verify_evidence_failed (%d).\n", ret);
        *claims = NULL;
        *claims_count = 0;
        OE_RAISE(OE_VERIFY_FAILED);
    }

    result = OE_OK;

done:
    return result;
}

void oe_free_claim_list(oe_claim_element_t* claims, size_t claims_count)
{
    if (claims)
    {
        // Free all memory.
        for (size_t i = 0; i < claims_count; i++)
            oe_free(claims[i].value);

        oe_free(claims);
    }
}
