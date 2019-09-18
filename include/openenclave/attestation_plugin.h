// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * @file attestation_plugin.h
 *
 * This file defines the programming interface for developing an
 * attestation plugin for supporting alternative evidence formats.
 *
 */

#ifndef _OE_ATTESTATION_PLUGIN_H
#define _OE_ATTESTATION_PLUGIN_H

#include <openenclave/bits/report.h>
#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

typedef struct _oe_quote_customization_plugin_context
    oe_quote_customization_plugin_context_t;

/**
 * Struct that defines the structure of each plugin. Each plugin must
 * define an UUID for its format and implement the functions in this
 * struct. Ideally, each plugin should provide a helper function to
 * create this struct on the behalf of the plugin users.
 */
typedef struct _oe_quote_customization_plugin_context
{
    /**
     * The UUID for the plugin.
     */
    uuid_t evidence_format_uuid;

    /**
     * The function that gets executed when a plugin is registered.
     *
     * @param[in] plugin_context A pointer to the attestation plugin struct.
     * @param[in] config_data An optional pointer to the configuration data.
     * @param[in] config_data_size The size in bytes of config_data.
     * @retval 0 on success and non-zero on error.
     * TODO(akagup): For these plugin functions, define what the error codes
     * should be or propagate them to the higher level oe_ functions.
     */
    int (*on_register)(
        oe_quote_customization_plugin_context_t* plugin_context,
        const void* config_data,
        size_t config_data_size);

    /**
     * The function that gets executed when a plugin is unregistered.
     *
     * @param[in] plugin_context A pointer to the attestation plugin struct.
     * @retval 0 on success and non-zero on error.
     * TODO(akagup): For these plugin functions, define what the error codes
     * should be or propagate them to the higher level oe_ functions.
     */
    int (*on_unregister)(
        oe_quote_customization_plugin_context_t* plugin_context);

    /**
     * Generates the attestation evidence. The caller may pass in
     * some custom user data, which must be attached to the evidence
     * and then cryptographically signed.
     *
     * @param[in] plugin context A pointer to the attestation plugin struct.
     * @param[in] user_data A buffer to the optional user_data.
     * @param[in] user_data_size The size in bytes of user_data.
     * @param[out] evidence_buffer An output pointer that will be assigned the
     * address of the evidence buffer.
     * @param[out] evidence_buffer_size A pointer that points to the size of the
     * evidence buffer.
     * @retval 0 on success and non-zero on error.
     * TODO(akagup): For these plugin functions, define what the error codes
     * should be or propagate them to the higher level oe_ functions.
     */
    int (*get_evidence)(
        oe_quote_customization_plugin_context_t* plugin_context,
        const uint8_t* user_data,
        size_t user_data_size,
        uint8_t** evidence_buffer,
        size_t* evidence_buffer_size);

    /**
     * Frees the generated attestation evidence.
     *
     * @param[in] plugin context A pointer to the attestation plugin struct.
     * @param[in] evidence_buffer A pointer to the evidence buffer.
     * @retval 0 on success and non-zero on error.
     * TODO(akagup): For these plugin functions, define what the error codes
     * should be or propagate them to the higher level oe_ functions.
     */
    int (*free_evidence)(
        oe_quote_customization_plugin_context_t* plugin_context,
        uint8_t* evidence_buffer);

    /**
     * Verifies the attestation evidence. The function also returns any custom
     * user data that was attached to the report and a list of key-value string
     * pairs, known as claims, which must contain the following known OE keys:
     *
     * TODO(akagup): Finalize this. Copied from the oe_identity_t struct.
     *    - id_version: Version of the OE claims. Must be 0.
     *    - security_version: Security version of the enclave. (ISVN for SGX).
     *    - attributes: Values of the attributes flags for the enclave:
     *        - OE_REPORT_ATTRIBUTES_DEBUG: The report is for a debug enclave.
     *        - OE_REPORT_ATTRIBUTES_REMOTE: The report can be used for remote
     *          attestation.
     *    - unique_id: The unique ID for the enclave (MRENCLAVE for SGX).
     *    - signer_id: The signer ID for the enclave (MRSIGNER for SGX).
     *    - product_id: The product ID for the enclave (ISVPRODID for SGX).
     *
     * @param[in] plugin_context A pointer to the attestation plugin struct.
     * @param[in] evidence_buffer The evidence buffer.
     * @param[in] evidence_buffer_size The size of evidence_buffer in bytes.
     * @param[out] claims The list of key-value string pairs.
     * @param[out] claims_count The number of elements in claims.
     * @param[out] user_data An output pointer that will be set to the address
     * inside the evidence buffer that points to the custom user data. If there
     * is none, then the pointer will be set to NULL.
     * @param[out] user_data_size A pointer that will be set to the size of
     * user_data in bytes. If there is no user_data, then the size will be 0.
     * @retval 0 on success and non-zero on error.
     * TODO(akagup): For these plugin functions, define what the error codes
     * should be or propagate them to the higher level oe_ functions.
     */
    int (*verify_evidence)(
        oe_quote_customization_plugin_context_t* plugin_context,
        const uint8_t* evidence_buffer,
        size_t evidence_buffer_size,
        oe_claim_element_t** claims,
        size_t* claim_count,
        uint8_t** user_data,
        size_t* user_data_size);
} oe_quote_customization_plugin_context_t;

/**
 * oe_register_attestation_plugin
 *
 * Registers a new attestation plugin and optionally configures it with plugin
 * specific configuration data. The function will fail if the plugin UUID has
 * already been registered.
 *
 * @param[in] plugin A pointer to the attestation plugin struct. Note that will
 * not copy the contents of the pointer, so the pointer must be kept valid until
 * the plugin is unregistered.
 * @param[in] config_data An optional pointer to the configuration data.
 * @param[in] config_data_size The size in bytes of config_data.
 * @retval OE_OK The function succeeded.
 * @retval OE_ALREADY_EXISTS A plugin with the same UUID is already registered.
 */
oe_result_t oe_register_attestation_plugin(
    oe_quote_customization_plugin_context_t* plugin,
    const void* config_data,
    size_t config_data_size);

/**
 * oe_unregister_attestation_plugin
 *
 * Unregisters an attestation plugin.
 *
 * @param[in] plugin A pointer to the attestation plugin struct.
 * @retval OE_OK The function succeeded.
 * @retval OE_NOT_FOUND The plugin does not exist.
 */
oe_result_t oe_unregister_attestation_plugin(
    oe_quote_customization_plugin_context_t* plugin);

/**
 * oe_get_attestation_evidence
 *
 * Generates the attestaton evidence for the given UUID attestation format.
 *
 * @param[in] evidence_format_uuid The UUID of the plugin.
 * @param[in] user_data Optional custom user data that will be attached and
 * cryptographically tied to the plugin evidence.
 * @param[in] user_data_size The size in bytes of user_data.
 * @param[out] evidence_buffer An output pointer that will be assigned the
 * address of the evidence buffer.
 * @param[out] evidence_buffer_size A pointer that points to the size of the
 * evidence buffer.
 * @retval OE_OK The function succeeded.
 * @retval OE_NOT_FOUND The plugin does not exist.
 */
oe_result_t oe_get_attestation_evidence(
    const uuid_t* evidence_format_uuid,
    const uint8_t* user_data,
    size_t user_data_size,
    uint8_t** evidence_buffer,
    size_t* evidence_buffer_size);

/**
 * oe_free_attestation_evidence
 *
 * Frees the attestation evidence.
 *
 * @param[in] evidence_buffer A pointer to the evidence buffer.
 */
void oe_free_attestation_evidence(uint8_t* evidence_buffer);

/**
 * oe_verify_attestation_evidence
 *
 * Verifies the attestation evidence. The function also returns any custom
 * user data that was attached to the report that the caller can verify
 * and a list of key-value pairs, known as claims, which will contain, at
 * minimum, the following known OE keys:
 *
 * TODO(akagup): Finalize this. These are copied from oe_identity_t.
 *    - id_version: Version of the OE claims. Must be 0.
 *    - security_version: Security version of the enclave. (ISVN for SGX).
 *    - attributes: Values of the attributes flags for the enclave:
 *        - OE_REPORT_ATTRIBUTES_DEBUG: The report is for a debug enclave.
 *        - OE_REPORT_ATTRIBUTES_REMOTE: The report can be used for remote
 *          attestation.
 *    - unique_id: The unique ID for the enclave (MRENCLAVE for SGX).
 *    - signer_id: The signer ID for the enclave (MRSIGNER for SGX).
 *    - product_id: The product ID for the enclave (ISVPRODID for SGX).
 *
 * @param[in] evidence_buffer The evidence buffer.
 * @param[in] evidence_buffer_size The size of evidence_buffer in bytes.
 * @param[out] claims The list of key-value string pairs.
 * @param[out] claims_count The number of elements in claims.
 * @param[out] user_data An output pointer that will be set to the address
 * inside the evidence buffer that points to the custom user data. If there
 * is none, then the pointer will be set to NULL.
 * @param[out] user_data_size A pointer that will be set to the size of
 * user_data in bytes. If there is no user_data, then the size will be 0.
 * @retval OE_OK The function succeeded.
 * @retval OE_NOT_FOUND The evidence requires a plugin that does not exist.
 */
oe_result_t oe_verify_attestation_evidence(
    const uint8_t* evidence_buffer,
    size_t evidence_buffer_size,
    oe_claim_element_t** claims,
    size_t* claims_count,
    uint8_t** user_data,
    size_t* user_data_size);

/**
 * oe_free_claims_list
 *
 * Frees a claims list.
 *
 * @param[in] claims The claims list.
 * @param[in] claims_count The number of claims in the claims list.
 */
void oe_free_claim_list(oe_claim_element_t* claims, size_t claims_count);

OE_EXTERNC_END

#endif /* _OE_ATTESTATION_PLUGIN_H */
