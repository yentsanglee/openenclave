#include "sha256.h"
#include <openenclave/internal/hexdump.h>
#include <stdio.h>
#include "endian.h"

void sha256_rorx(void* input_data, uint32_t digest[8], uint64_t num_blks);

void sha256_transform(void* input_data, uint32_t digest[8], uint64_t num_blks);

#if defined(USE_SHA256_RORX)
#define SHA256 sha256_rorx
#else
#define SHA256 sha256_transform
#endif

OE_INLINE void _digest_to_hash(sha256_t* hash, uint32_t digest[8])
{
    hash->u.u32[0] = oe_to_big_endian_u32(digest[0]);
    hash->u.u32[1] = oe_to_big_endian_u32(digest[1]);
    hash->u.u32[2] = oe_to_big_endian_u32(digest[2]);
    hash->u.u32[3] = oe_to_big_endian_u32(digest[3]);
    hash->u.u32[4] = oe_to_big_endian_u32(digest[4]);
    hash->u.u32[5] = oe_to_big_endian_u32(digest[5]);
    hash->u.u32[6] = oe_to_big_endian_u32(digest[6]);
    hash->u.u32[7] = oe_to_big_endian_u32(digest[7]);
}

int sha256(sha256_t* hash, const void* data, size_t size)
{
    int ret = -1;
    const uint8_t* p = (const uint8_t*)data;
    const size_t blksz = 64;
    size_t n = size / blksz;
    uint32_t digest[8] = {
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
        } final_block_t;
        final_block_t final_block = {
            {0x80}, 0,
        };

        final_block.size = oe_to_big_endian_u64((uint64_t)size << 3);
        SHA256((void*)&final_block, digest, 1);
        _digest_to_hash(hash, digest);
    }

    ret = 0;

done:
    return ret;
}

void sha256_64(sha256_t* hash, const void* data)
{
    uint32_t digest[8] = {
        0x6a09e667ul,
        0xbb67ae85ul,
        0x3c6ef372ul,
        0xa54ff53aul,
        0x510e527ful,
        0x9b05688cul,
        0x1f83d9abul,
        0x5be0cd19ul,
    };
    OE_ALIGNED(8)
    static const uint8_t _final[] = {
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    };

    SHA256((void*)data, digest, 1);
    SHA256((void*)_final, digest, 1);
    _digest_to_hash(hash, digest);
}
