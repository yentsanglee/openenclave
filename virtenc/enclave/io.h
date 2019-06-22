// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_IO_H
#define _VE_IO_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

ssize_t ve_read(int fd, void* buf, size_t count);

ssize_t ve_write(int fd, const void* buf, size_t count);

int ve_close(int fd);

#endif /* _VE_IO_H */
