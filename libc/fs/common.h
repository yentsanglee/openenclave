#ifndef _FS_DEFS_H
#define _FS_DEFS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define FS_PATH_MAX 256

#define FS_BLOCK_SIZE 512

#define FS_INLINE static __inline

#define FS_COUNTOF(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#endif /* _FS_DEFS_H */
