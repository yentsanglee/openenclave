// clang-format off
#include "common.h"
#include "sgx_error.h"
#include "sgx_tcrypto.h"
// clang-format on

sgx_status_t SGXAPI sgx_rijndael128GCM_encrypt(
    const sgx_aes_gcm_128bit_key_t* p_key,
    const uint8_t* p_src,
    uint32_t src_len,
    uint8_t* p_dst,
    const uint8_t* p_iv,
    uint32_t iv_len,
    const uint8_t* p_aad,
    uint32_t aad_len,
    sgx_aes_gcm_128bit_tag_t* p_out_mac)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGXAPI sgx_rijndael128GCM_decrypt(
    const sgx_aes_gcm_128bit_key_t* p_key,
    const uint8_t* p_src,
    uint32_t src_len,
    uint8_t* p_dst,
    const uint8_t* p_iv,
    uint32_t iv_len,
    const uint8_t* p_aad,
    uint32_t aad_len,
    const sgx_aes_gcm_128bit_tag_t* p_in_mac)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t SGXAPI sgx_rijndael128_cmac_msg(
    const sgx_cmac_128bit_key_t* p_key,
    const uint8_t* p_src,
    uint32_t src_len,
    sgx_cmac_128bit_tag_t* p_mac)
{
    /* TODO: */
    return SGX_ERROR_UNEXPECTED;
}
