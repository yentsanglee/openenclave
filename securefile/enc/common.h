#ifndef _OE_SGXFS_COMMON_H
#define _OE_SGXFS_COMMON_H

typedef void FILE;
#define __DEFINED_FILE

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef int errno_t;

int consttime_memequal(const void* b1, const void* b2, size_t len);

errno_t memset_s(void* s, size_t smax, int c, size_t n);

#if defined(__cplusplus)
}
#endif

#endif /* _OE_SGXFS_COMMON_H */
