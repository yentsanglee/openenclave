// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/sys/stat.h>
#include <sys/stat.h>
#include "check_field.h"

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
