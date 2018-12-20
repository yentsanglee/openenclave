// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_MUXFS_H
#define _OE_MUXFS_H

#include <openenclave/bits/fs.h>

OE_EXTERNC_BEGIN

extern oe_fs_t oe_muxfs;

int oe_muxfs_register_fs(oe_fs_t* muxfs, const char* path, oe_fs_t* fs);

int oe_muxfs_unregister_fs(oe_fs_t* muxfs, const char* path);

OE_EXTERNC_END

#endif /* _OE_MUXFS_H */
