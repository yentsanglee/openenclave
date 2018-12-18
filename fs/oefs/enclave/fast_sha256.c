#include "endian.h"
#include "fast_sha256.h"
#include "sha.h"
#include <stdio.h>

#define USE_RORX

#if defined(USE_SSE4)
    #define SHA256 sha256_sse4
#elif defined(USE_RORX)
    #define SHA256 sha256_rorx
#elif defined(USE_AVX)
    #define SHA256 sha256_avx
#elif defined(USE_RORX_X8MS)
    #define SHA256 sha256_rorx_x8ms
#else
#error "please define USE_SSE4, USE_RORX, or USE_AVX"
#endif

void sha256_sse4(void *input_data, uint32_t digest[8], uint64_t num_blks);

void sha256_rorx(void *input_data, uint32_t digest[8], uint64_t num_blks);

void sha256_avx(void *input_data, uint32_t digest[8], uint64_t num_blks);

void sha256_rorx_x8ms(void *input_data, uint32_t digest[8], uint64_t num_blks);

int fast_sha256(uint8_t hash[32], const void* data, size_t size)
{
    int ret = -1;
    const uint8_t* p = (const uint8_t*)data;
    const size_t blksz = 64;
    size_t n = size / blksz;
    uint32_t digest[8] =
    {
        0x6a09e667ul,
        0xbb67ae85ul,
        0x3c6ef372ul,
        0xa54ff53aul,
        0x510e527ful,
        0x9b05688cul,
        0x1f83d9abul,
        0x5be0cd19ul,
    };

    if (size % blksz)
        goto done;

    SHA256((void*)p, digest, n);

    /* Write the final block. */
    {
        typedef struct _final_block
        {
            uint8_t data[64 - sizeof(uint64_t)];
            uint64_t size;
        }
        final_block_t;
        final_block_t final_block =
        {
            { 0x80 },
            0,
        };
        uint32_t* p = (uint32_t*)hash;

        final_block.size = oe_to_big_endian_u64((uint64_t)size << 3);
        SHA256((void*)&final_block, digest, 1);

        for (size_t i = 0; i < OE_COUNTOF(digest); i++)
            *p++ = oe_to_big_endian_u32(digest[i]);
    }

    ret = 0;

done:
    return ret;
}
