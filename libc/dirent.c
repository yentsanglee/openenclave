// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <dirent.h>
#include <openenclave/corelibc/dirent.h>
#include <openenclave/internal/defs.h>

#define SIZEOF_F(T, F) sizeof(((T*)0)->F)

#define CHECK_FIELD(T1, T2, F)                                  \
    OE_STATIC_ASSERT(OE_OFFSETOF(T1, F) == OE_OFFSETOF(T2, F)); \
    OE_STATIC_ASSERT(sizeof(((T1*)0)->F) == sizeof(((T2*)0)->F));

/*
**==============================================================================
**
** Verify that oe_dirent and dirent have same layout.
**
**==============================================================================
*/

OE_STATIC_ASSERT(sizeof(struct oe_dirent) == sizeof(struct dirent));
CHECK_FIELD(struct oe_dirent, struct dirent, d_ino)
CHECK_FIELD(struct oe_dirent, struct dirent, d_off)
CHECK_FIELD(struct oe_dirent, struct dirent, d_reclen)
CHECK_FIELD(struct oe_dirent, struct dirent, d_type)
CHECK_FIELD(struct oe_dirent, struct dirent, d_name)
