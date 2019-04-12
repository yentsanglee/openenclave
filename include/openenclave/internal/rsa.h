// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_RSA_H
#define _OE_RSA_H

#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>
#include "hash.h"
#include "sha.h"

OE_EXTERNC_BEGIN

/* Opaque representation of a private RSA key */
typedef struct _oe_rsa_private_key
{
    /* Internal implementation */
    uint64_t impl[4];
} oe_rsa_private_key_t;

/* Opaque representation of a public RSA key */
typedef struct _oe_rsa_public_key
{
    /* Internal implementation */
    uint64_t impl[4];
} oe_rsa_public_key_t;

/**
 * Reads a private RSA key from PEM data
 *
 * This function reads a private RSA key from PEM data with the following PEM
 * headers.
 *
 *     -----BEGIN RSA PRIVATE KEY-----
 *     ...
 *     -----END RSA PRIVATE KEY-----
 *
 * The caller is responsible for releasing the key by passing it to
 * oe_rsa_private_key_free().
 *
 * @param private_key initialized key handle upon return
 * @param pem_data zero-terminated PEM data
 * @param pem_size size of the PEM data (including the zero-terminator)
 *
 * @return OE_OK upon success
 */
oe_result_t oe_rsa_private_key_read_pem(
    oe_rsa_private_key_t* private_key,
    const uint8_t* pem_data,
    size_t pem_size);

/**
 * Reads a public RSA key from PEM data
 *
 * This function reads a public RSA key from PEM data with the following PEM
 * headers.
 *
 *     -----BEGIN PUBLIC KEY-----
 *     ...
 *     -----END PUBLIC KEY-----
 *
 * The caller is responsible for releasing the key by passing it to
 * oe_rsa_public_key_free().
 *
 * @param public_key initialized key handle upon return
 * @param pem_data zero-terminated PEM data
 * @param pem_size size of the PEM data (including the zero-terminator)
 *
 * @return OE_OK upon success
 */
oe_result_t oe_rsa_public_key_read_pem(
    oe_rsa_public_key_t* public_key,
    const uint8_t* pem_data,
    size_t pem_size);

/**
 * Writes a private RSA key to PEM format
 *
 * This function writes a private RSA key to PEM data with the following PEM
 * headers.
 *
 *     -----BEGIN RSA PRIVATE KEY-----
 *     ...
 *     -----END RSA PRIVATE KEY-----
 *
 * @param private_key key to be written
 * @param pem_data buffer where PEM data will be written
 * @param[in,out] pem_size buffer size (in); PEM data size (out)
 *
 * @return OE_OK upon success
 * @return OE_BUFFER_TOO_SMALL PEM buffer is too small
 */
oe_result_t oe_rsa_private_key_write_pem(
    const oe_rsa_private_key_t* private_key,
    uint8_t* pem_data,
    size_t* pem_size);

/**
 * Writes a public RSA key to PEM format
 *
 * This function writes a public RSA key to PEM data with the following PEM
 * headers.
 *
 *     -----BEGIN PUBLIC KEY-----
 *     ...
 *     -----END PUBLIC KEY-----
 *
 * @param public_key key to be written
 * @param pem_data buffer where PEM data will be written
 * @param[in,out] pem_size buffer size (in); PEM data size (out)
 *
 * @return OE_OK upon success
 * @return OE_BUFFER_TOO_SMALL PEM buffer is too small
 */
oe_result_t oe_rsa_public_key_write_pem(
    const oe_rsa_public_key_t* public_key,
    uint8_t* pem_data,
    size_t* pem_size);

/**
 * Releases an RSA private key
 *
 * This function releases the given RSA private key.
 *
 * @param key handle of key being released
 *
 * @return OE_OK upon success
 */
oe_result_t oe_rsa_private_key_free(oe_rsa_private_key_t* private_key);

/**
 * Releases an RSA public key
 *
 * This function releases the given RSA public key.
 *
 * @param key handle of key being released
 *
 * @return OE_OK upon success
 */
oe_result_t oe_rsa_public_key_free(oe_rsa_public_key_t* public_key);

/**
 * Digitally encrypts a message with a public RSA key
 *
 * This function uses a public RSA key to encrypt a message using RSA OAEP
 * SHA 256.
 *
 * @param public_key The public RSA key
 * @param plain The plaintext buffer
 * @param plain_size the plaintext buffer size
 * @param cipher The ciphertext
 * @param[in,out] cipher_size The buffer size (in); ciphertext size (out)
 *
 * @return OE_OK on success
 * @return OE_BUFFER_TOO_SMALL signature buffer is too small
 */
oe_result_t oe_rsa_public_key_encrypt(
    const oe_rsa_public_key_t* public_key,
    const uint8_t* plain,
    size_t plain_size,
    uint8_t* cipher,
    size_t* cipher_size);

/**
 * Digitally signs a message with a private RSA key
 *
 * This function uses a private RSA key to sign a message with the given hash.
 *
 * @param private_key private RSA key of signer
 * @param hash_type type of hash parameter
 * @param hash_data hash of the message being signed
 * @param hash_size size of the hash data
 * @param signature signature buffer
 * @param[in,out] signature_size buffer size (in); signature size (out)
 *
 * @return OE_OK on success
 * @return OE_BUFFER_TOO_SMALL signature buffer is too small
 */
oe_result_t oe_rsa_private_key_sign(
    const oe_rsa_private_key_t* private_key,
    oe_hash_type_t hash_type,
    const void* hash_data,
    size_t hash_size,
    uint8_t* signature,
    size_t* signature_size);

/**
 * Digitally decrypts a message with a private RSA key
 *
 * This function uses a private RSA key to decrypt a message using RSA
 * OAEP SHA256.
 *
 * @param private_key The private RSA key
 * @param cipher The ciphertext
 * @param cipher_size The size of the ciphertext buffer.
 * @param plain The plaintext buffer.
 * @param[in,out] plain_size The buffer size (in); plaintext size (out)
 *
 * @return OE_OK on success
 * @return OE_BUFFER_TOO_SMALL signature buffer is too small
 */
oe_result_t oe_rsa_private_key_decrypt(
    const oe_rsa_private_key_t* private_key,
    const uint8_t* cipher,
    size_t cipher_size,
    uint8_t* plain,
    size_t* plain_size);

/**
 * Verifies that a message was signed by an RSA key
 *
 * This function verifies that the message with the given hash was signed by the
 * given RSA key.
 *
 * @param public_key public RSA key of signer
 * @param hash_type type of hash parameter
 * @param hash_data hash of the signed message
 * @param hash_size size of the hash data
 * @param signature expected signature
 * @param signature_size size of the expected signature
 *
 * @return OE_OK if the message was signed with the given certificate
 */
oe_result_t oe_rsa_public_key_verify(
    const oe_rsa_public_key_t* public_key,
    oe_hash_type_t hash_type,
    const void* hash_data,
    size_t hash_size,
    const uint8_t* signature,
    size_t signature_size);

/**
 * Generates an RSA private-public key pair
 *
 * This function generates an RSA private-public key pair from the given
 * parameters.
 *
 * @param bits the number of bits in the key
 * @param exponent the exponent for this key
 * @param private_key generated private key
 * @param public_key generated public key
 *
 * @return OE_OK on success
 */
oe_result_t oe_rsa_generate_key_pair(
    uint64_t bits,
    uint64_t exponent,
    oe_rsa_private_key_t* private_key,
    oe_rsa_public_key_t* public_key);

/**
 * Get the modulus from a public RSA key.
 *
 * This function gets the modulus from a public RSA key. The modulus is
 * written to **buffer**.
 *
 * @param public_key key whose modulus is fetched.
 * @param buffer buffer where modulus is written (may be null).
 * @param buffer_size[in,out] buffer size on input; actual size on output.
 *
 * @return OE_OK upon success
 * @return OE_BUFFER_TOO_SMALL buffer is too small and **buffer_size** contains
 *         the required size.
 */
oe_result_t oe_rsa_public_key_get_modulus(
    const oe_rsa_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size);

/**
 * Get the exponent from a public RSA key.
 *
 * This function gets the exponent from a public RSA key. The exponent is
 * written to **buffer**.
 *
 * @param public_key key whose exponent is fetched.
 * @param buffer buffer where exponent is written (may be null).
 * @param buffer_size[in,out] buffer size on input; actual size on output.
 *
 * @return OE_OK upon success
 * @return OE_BUFFER_TOO_SMALL buffer is too small and **buffer_size** contains
 *         the required size.
 */
oe_result_t oe_rsa_public_key_get_exponent(
    const oe_rsa_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size);

/**
 * Get the parameters from a private RSA key.
 *
 * This function gets the modulus, pq primes, the public exponent and the
 * private exponent. These are written to the specified buffers.
 *
 * @param private_key The input private key
 * @param m The modulus buffer
 * @param m_size[in, out] The modulus buffer size on input; actual size on
 * output.
 * @param p The prime p buffer
 * @param p_size[in, out] The prime p buffer size on input; actual size on
 * output.
 * @param q The prime q buffer
 * @param q_size[in, out] The prime q buffer size on input; actual size on
 * output.
 * @param d The private exponent buffer
 * @param d_size[in, out] The private exponent buffer size on input; actual size
 * on output.
 * @param e The public exponent buffer
 * @param e_size[in, out] The public exponent buffer size on input; actual size
 * on output.
 *
 * @return OE_OK upon success
 * @return OE_BUFFER_TOO_SMALL a buffer is too small and the size parameters
 * contain the required sizes.
 */
oe_result_t oe_rsa_private_key_get_parameters(
    const oe_rsa_private_key_t* private_key,
    uint8_t* m,
    size_t* m_size,
    uint8_t* p,
    size_t* p_size,
    uint8_t* q,
    size_t* q_size,
    uint8_t* d,
    size_t* d_size,
    uint8_t* e,
    size_t* e_size);

/**
 * Get the parameters from a private RSA key.
 *
 * This function gets the parameters for RSA decryption and signing using the
 * remainder theorem.
 *
 * @param private_key The input private key
 * @param dp The buffer containing dp = d mod (p -1)
 * @param dp_size[in, out] The dp buffer size on input; actual size on output.
 * @param dq The buffer containing dq = d mod (q - 1)
 * @param dq_size[in, out] The dq buffer size on input; actual size on output.
 * @param qinv The buffer containing qinv = q^-1 mod p
 * @param qinv_size[in, out] The qinv buffer size on input; actual size on
 * output.
 *
 * @return OE_OK upon success
 * @return OE_BUFFER_TOO_SMALL a buffer is too small and the size parameters
 * contain the required sizes.
 */
oe_result_t oe_rsa_private_key_get_crt_parameters(
    const oe_rsa_private_key_t* private_key,
    uint8_t* dp,
    size_t* dp_size,
    uint8_t* dq,
    size_t* dq_size,
    uint8_t* qinv,
    size_t* qinv_size);

/**
 * Load the parameters from a RSA public key.
 *
 * This function loads a RSA public key using the modulus and public expoent.
 *
 * @param public_key The output public key
 * @param m The modulus buffer
 * @param m_size The modulus buffer size
 * @param e The exponent buffer
 * @param e_size The exponent buffer size
 * @return OE_OK upon success
 */
oe_result_t oe_rsa_public_key_load_parameters(
    oe_rsa_public_key_t* public_key,
    const uint8_t* m,
    size_t m_size,
    const uint8_t* e,
    size_t e_size);

/**
 * Load the parameters from a RSA private key.
 *
 * This function loads a RSA private key using the pq primes and
 * the public exponent.
 *
 * @param private_key The output private key
 * @param p The p prime buffer
 * @param p_size The p prime buffer size
 * @param q The q prime buffer
 * @param q_size The q prime buffer size
 * @param e The public exponent buffer
 * @param e_size The exponent buffer size
 * @return OE_OK upon success
 */
oe_result_t oe_rsa_private_key_load_parameters(
    oe_rsa_private_key_t* private_key,
    const uint8_t* p,
    size_t p_size,
    const uint8_t* q,
    size_t q_size,
    const uint8_t* e,
    size_t e_size);

/**
 * Determine whether two RSA public keys are identical.
 *
 * This function determines whether two RSA public keys are identical.
 *
 * @param public_key1 first key.
 * @param public_key2 second key.
 * @param equal[out] true if the keys are identical.
 *
 * @return OE_OK successful and **equal** is either true or false.
 * @return OE_INVALID_PARAMETER a parameter was invalid.
 *
 */
oe_result_t oe_rsa_public_key_equal(
    const oe_rsa_public_key_t* public_key1,
    const oe_rsa_public_key_t* public_key2,
    bool* equal);

OE_EXTERNC_END

#endif /* _OE_RSA_H */
