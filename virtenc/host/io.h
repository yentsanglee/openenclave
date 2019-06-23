// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_IO_H
#define _VE_IO_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

int ve_readn(int fd, void* buf, size_t count);

int ve_writen(int fd, const void* buf, size_t count);

#endif /* _VE_IO_H */
