// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_HOSTFS_H
#define _OE_HOSTFS_H

#include <openenclave/bits/fs.h>

OE_EXTERNC_BEGIN

extern oe_fs_t oe_hostfs;

void oe_install_hostfs(void);

OE_EXTERNC_END

#endif /* _OE_HOSTFS_H */
