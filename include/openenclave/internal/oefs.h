#ifndef _OE_OEFS_COMMON_H
#define _OE_OEFS_COMMON_H

#include <openenclave/bits/fs.h>

OE_EXTERNC_BEGIN

#define OEFS_KEY_SIZE 32

#define OEFS_BLOCK_SIZE 1024

#define OEFS_FLAG_NONE 0
#define OEFS_FLAG_MKFS 1
#define OEFS_FLAG_CRYPTO 2
#define OEFS_FLAG_AUTH_CRYPTO 4
#define OEFS_FLAG_INTEGRITY 8
#define OEFS_FLAG_CACHING 16

int oefs_calculate_total_blocks(size_t nblks, size_t* total_nblks);

int oe_oefs_mkfs(const char* source, const uint8_t key[OEFS_KEY_SIZE]);

int oe_oefs_initialize(
    oe_fs_t* fs,
    const char* source,
    const uint8_t key[OEFS_KEY_SIZE]);

void oe_install_oefs(void);

OE_EXTERNC_END

#endif /* _OE_OEFS_COMMON_H */
