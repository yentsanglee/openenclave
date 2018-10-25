// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_FILE_H
#define _OE_FILE_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

typedef struct _oe_file oe_file_t;

typedef unsigned mode_t;

oe_file_t* oe_file_open(const char* path, int flags, mode_t mode);

ssize_t oe_file_read(oe_file_t* file, void* buf, size_t count);

ssize_t oe_file_write(oe_file_t* file, const void* buf, size_t count);

int oe_file_close(oe_file_t* file);

OE_EXTERNC_END

#endif /* _OE_FILE_H */
