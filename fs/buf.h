// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_BUF_H
#define _FS_BUF_H

#include <stddef.h>
#include <stdint.h>
#include "common.h"

FS_EXTERN_C_BEGIN

/*
**==============================================================================
**
** fs_buf_t:
**
**==============================================================================
*/

#define BUF_INITIALIZER \
    {                   \
        NULL, 0, 0      \
    }

typedef struct _fs_buf
{
    void* data;
    uint32_t size;
    uint32_t cap;
} fs_buf_t;

void fs_buf_release(fs_buf_t* buf);

int fs_buf_clear(fs_buf_t* buf);

int fs_buf_reserve(fs_buf_t* buf, uint32_t cap);

int fs_buf_resize(fs_buf_t* buf, uint32_t new_size);

int fs_buf_append(fs_buf_t* buf, const void* data, uint32_t size);

/*
**==============================================================================
**
** fs_bufu32_t:
**
**==============================================================================
*/

#define BUF_U32_INITIALIZER \
    {                       \
        NULL, 0, 0          \
    }

typedef struct _bufu32
{
    uint32_t* data;
    uint32_t size;
    uint32_t cap;
} fs_bufu32_t;

void fs_bufu32_release(fs_bufu32_t* buf);

void fs_bufu32_clear(fs_bufu32_t* buf);

int fs_bufu32_resize(fs_bufu32_t* buf, uint32_t new_size);

int fs_bufu32_append(fs_bufu32_t* buf, const uint32_t* data, uint32_t size);

FS_EXTERN_C_END

#endif /* _FS_BUF_H */
