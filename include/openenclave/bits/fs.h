// Copyright (c) Microsoft Corporation. All rights reserved._ops
// Licensed under the MIT License.

#ifndef _OE_BITS_FS_H
#define _OE_BITS_FS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/device.h>
#include <openenclave/bits/types.h>
#include <openenclave/corelibc/dirent.h>
#include <openenclave/corelibc/fcntl.h>
#include <openenclave/corelibc/limits.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/sys/mount.h>
#include <openenclave/corelibc/sys/stat.h>
#include <openenclave/corelibc/sys/uio.h>
#include <openenclave/corelibc/unistd.h>

OE_EXTERNC_BEGIN

/* Mount flags. */
#define OE_MOUNT_RDONLY 1

OE_EXTERNC_END

#endif // _OE_BITS_FS_H
