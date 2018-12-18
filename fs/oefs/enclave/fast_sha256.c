#include "sha.h"
#include <mbedtls/sha256.h>

void sha256_sse4(void *input_data, uint32_t digest[8], uint64_t num_blks);

void sha256_rorx(void *input_data, uint32_t digest[8], uint64_t num_blks);

void sha256_avx(void *input_data, uint32_t digest[8], uint64_t num_blks);

int fast_sha256(
    oefs_sha256_t* hash, 
    const void* data, 
    size_t size)
{
    int ret = -1;
    const uint8_t* p = (const uint8_t*)data;
    const size_t blksz = 64;
    size_t n = size / blksz;
    mbedtls_sha256_context c =
    {
        .total = { 0 },
        .state = 
        {
            0x6a09e667ul,
            0xbb67ae85ul,
            0x3c6ef372ul,
            0xa54ff53aul,
            0x510e527ful,
            0x9b05688cul,
            0x1f83d9abul,
            0x5be0cd19ul,
        },
        .buffer = 
        {
            0,
        },
        .is224 = 0,
    };

    if (size % blksz)
        goto done;

#if 0
    sha256_sse4((void*)p, c.state, n);
#elif 1
    sha256_rorx((void*)p, c.state, n);
#else
    sha256_avx((void*)p, c.state, n);
#endif
    c.total[0] = size;

    if (mbedtls_sha256_finish_ret(&c, hash->data) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}
