// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_ARGS_H
#define _FS_ARGS_H

#include "common.h"
#include "guid.h"

FS_EXTERN_C_BEGIN

typedef struct _fs_args
{
    fs_guid_t guid;
} fs_args_t;

FS_EXTERN_C_END

#endif /* _FS_ARGS_H */
