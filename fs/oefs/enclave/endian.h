#ifndef _OEFS_ENDIAN_H
#define _OEFS_ENDIAN_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_INLINE bool oe_is_big_endian()
{
#if defined(__i386) || defined(__x86_64)
    return false;
#else
    typedef union _U {
        unsigned short x;
        unsigned char bytes[2];
    } U;
    static U u = {0xABCD};
    return u.bytes[0] == 0xAB;
#endif
}

OE_INLINE uint64_t oe_to_big_endian_u64(uint64_t x)
{
    if (oe_is_big_endian())
    {
        return x;
    }
    else
    {
        return ((uint64_t)((x & 0xFF) << 56)) |
               ((uint64_t)((x & 0xFF00) << 40)) |
               ((uint64_t)((x & 0xFF0000) << 24)) |
               ((uint64_t)((x & 0xFF000000) << 8)) |
               ((uint64_t)((x & 0xFF00000000) >> 8)) |
               ((uint64_t)((x & 0xFF0000000000) >> 24)) |
               ((uint64_t)((x & 0xFF000000000000) >> 40)) |
               ((uint64_t)((x & 0xFF00000000000000) >> 56));
    }
}

OE_INLINE uint32_t oe_to_big_endian_u32(uint32_t x)
{
    if (oe_is_big_endian())
    {
        return x;
    }
    else
    {
        return ((uint32_t)((x & 0x000000FF) << 24)) |
               ((uint32_t)((x & 0x0000FF00) << 8)) |
               ((uint32_t)((x & 0x00FF0000) >> 8)) |
               ((uint32_t)((x & 0xFF000000) >> 24));
    }
}

#endif /* _OEFS_ENDIAN_H */
