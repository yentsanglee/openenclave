#ifndef _OE_OEFS_COMMON_H
#define _OE_OEFS_COMMON_H

#include <openenclave/internal/fs.h>

oe_result_t oe_oefs_initialize(
    oe_fs_t** fs_out,
    const char* source,
    uint32_t flags,
    size_t nblks,
    const uint8_t key[OEFS_KEY_SIZE]);

#endif /* _OE_OEFS_COMMON_H */
