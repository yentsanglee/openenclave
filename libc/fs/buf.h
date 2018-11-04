// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _FS_BUF_H
#define _FS_BUF_H

#include <stddef.h>
#include <stdint.h>

/*
**==============================================================================
**
** buf_t:
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
} buf_t;

void buf_release(buf_t* buf);

int buf_clear(buf_t* buf);

int buf_reserve(buf_t* buf, uint32_t cap);

int buf_resize(buf_t* buf, uint32_t new_size);

int buf_append(buf_t* buf, const void* data, uint32_t size);

/*
**==============================================================================
**
** buf_u32_t:
**
**==============================================================================
*/

#define BUF_U32_INITIALIZER \
    {                       \
        NULL, 0, 0          \
    }

typedef struct _buf_u32
{
    uint32_t* data;
    uint32_t size;
    uint32_t cap;
} buf_u32_t;

void buf_u32_release(buf_u32_t* buf);

void buf_u32_clear(buf_u32_t* buf);

int buf_u32_resize(buf_u32_t* buf, uint32_t new_size);

int buf_u32_append(buf_u32_t* buf, const uint32_t* data, uint32_t size);

#endif /* _FS_BUF_H */
