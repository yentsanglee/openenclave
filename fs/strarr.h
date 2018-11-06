// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_STRARR_H
#define _FS_STRARR_H

#include "common.h"

#define FS_STRARR_INITIALIZER \
    {                         \
        NULL, 0, 0            \
    }

typedef struct _fs_strarr
{
    char** data;
    size_t size;
    size_t capacity;
} fs_strarr_t;

void fs_strarr_release(fs_strarr_t* self);

int fs_strarr_append(fs_strarr_t* self, const char* data);

int fs_strarr_remove(fs_strarr_t* self, size_t index);

#endif /* _FS_STRARR_H */
