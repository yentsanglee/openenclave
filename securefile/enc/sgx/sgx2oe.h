
#include <openenclave/enclave.h>
#include <openenclave/internal/thread.h>
#include <openenclave/bits/safecrt.h>
//#include <openenclave/internal/keys.h>
#define sgx_thread_mutex_unlock oe_mutex_unlock
#define sgx_thread_mutex_lock oe_mutex_lock
#define sgx_thread_mutex_t oe_mutex_t
#define sgx_thread_mutex_init(a,b) oe_mutex_init(a)
#define sgx_thread_mutex_destroy oe_mutex_destroy
#define sgx_read_rand oe_random
#define sgx_is_outside_enclave oe_is_outside_enclave
#define memset_s oe_memset_s
#define sgx_get_key oe_get_key
//#define sgx_rijndael128GCM_decrypt oe_rijndael128GCM_decrypt
//#define sgx_rijndael128GCM_encrypt oe_rijndael128GCM_encrypt
//#define sgx_rijndael128_cmac_msg oe_rijndael128_cmac_msg
//#define sgx_aes_gcm_128bit_key_t oe_aes_gcm_128bit_key_t
//#define sgx_aes_gcm_128bit_tag_t oe_aes_gcm_128bit_tag_t
//#define SGX_AESGCM_MAC_SIZE OE_AESGCM_MAC_SIZE
//#define SGX_AESGCM_KEY_SIZE OE_AESGCM_KEY_SIZE
//#define SGX_AESGCM_IV_SIZE OE_AESGCM_IV_SIZE

#define OE_AESGCM_MAC_SIZE             16
#define OE_AESGCM_KEY_SIZE             16
#define OE_AESGCM_IV_SIZE              12
typedef uint8_t oe_aes_gcm_128bit_key_t[OE_AESGCM_KEY_SIZE];
typedef uint8_t oe_aes_gcm_128bit_tag_t[OE_AESGCM_MAC_SIZE];

#define OE_CMAC_MAC_SIZE               16
#define OE_CMAC_KEY_SIZE               16
typedef uint8_t oe_cmac_128bit_key_t[OE_CMAC_KEY_SIZE];
typedef uint8_t oe_cmac_128bit_tag_t[OE_CMAC_MAC_SIZE];

oe_result_t oe_rijndael128_cmac_msg(const oe_cmac_128bit_key_t *p_key, const uint8_t *p_src,
                                      uint32_t src_len, oe_cmac_128bit_tag_t *p_mac);
oe_result_t oe_rijndael128GCM_encrypt(const oe_aes_gcm_128bit_key_t *p_key, const uint8_t *p_src, uint32_t src_len,
                                        uint8_t *p_dst, const uint8_t *p_iv, uint32_t iv_len, const uint8_t *p_aad, uint32_t aad_len,
                                        oe_aes_gcm_128bit_tag_t *p_out_mac);
oe_result_t oe_rijndael128GCM_decrypt(const oe_aes_gcm_128bit_key_t *p_key, const uint8_t *p_src,
                                        uint32_t src_len, uint8_t *p_dst, const uint8_t *p_iv, uint32_t iv_len,
                                        const uint8_t *p_aad, uint32_t aad_len, const oe_aes_gcm_128bit_tag_t *p_in_mac);
