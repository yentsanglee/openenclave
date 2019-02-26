// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/sys/stat.h>
#include <openenclave/internal/defs.h>
#include <sys/stat.h>

#define SIZEOF_F(T, F) sizeof(((T*)0)->F)

#define CHECK_FIELD(T1, T2, F)                                  \
    OE_STATIC_ASSERT(OE_OFFSETOF(T1, F) == OE_OFFSETOF(T2, F)); \
    OE_STATIC_ASSERT(sizeof(((T1*)0)->F) == sizeof(((T2*)0)->F));

/*
**==============================================================================
**
** Verify that oe_stat and stat have same layout.
**
**==============================================================================
*/

OE_STATIC_ASSERT(sizeof(struct oe_stat) > 0);

OE_STATIC_ASSERT(sizeof(struct oe_stat) == sizeof(struct stat));
CHECK_FIELD(struct oe_stat, struct stat, st_dev)
CHECK_FIELD(struct oe_stat, struct stat, st_ino)
CHECK_FIELD(struct oe_stat, struct stat, st_nlink)
CHECK_FIELD(struct oe_stat, struct stat, st_mode)
CHECK_FIELD(struct oe_stat, struct stat, st_uid)
CHECK_FIELD(struct oe_stat, struct stat, st_gid)
CHECK_FIELD(struct oe_stat, struct stat, st_rdev)
CHECK_FIELD(struct oe_stat, struct stat, st_size)
CHECK_FIELD(struct oe_stat, struct stat, st_blksize)
CHECK_FIELD(struct oe_stat, struct stat, st_blocks)
CHECK_FIELD(struct oe_stat, struct stat, st_atim)
CHECK_FIELD(struct oe_stat, struct stat, st_mtim)
CHECK_FIELD(struct oe_stat, struct stat, st_ctim)
