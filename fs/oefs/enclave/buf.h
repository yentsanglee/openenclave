// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_BUF_H
#define _OEFS_BUF_H

#include <stddef.h>
#include <stdint.h>
#include "common.h"

OE_EXTERNC_BEGIN

/*
**==============================================================================
**
** oefs_buf_t:
**
**==============================================================================
*/

#define OEFS_BUF_INITIALIZER \
    {                        \
        NULL, 0, 0           \
    }

typedef struct _oefs_buf
{
    void* data;
    uint32_t size;
    uint32_t cap;
} oefs_buf_t;

void oefs_buf_release(oefs_buf_t* buf);

int oefs_buf_clear(oefs_buf_t* buf);

int oefs_buf_reserve(oefs_buf_t* buf, uint32_t cap);

int oefs_buf_resize(oefs_buf_t* buf, uint32_t new_size);

int oefs_buf_append(oefs_buf_t* buf, const void* data, uint32_t size);

/*
**==============================================================================
**
** oefs_bufu32_t:
**
**==============================================================================
*/

#define OEFS_BUF_U32_INITIALIZER \
    {                            \
        NULL, 0, 0               \
    }

typedef struct _oefs_bufu32
{
    uint32_t* data;
    uint32_t size;
    uint32_t cap;
} oefs_bufu32_t;

void oefs_bufu32_release(oefs_bufu32_t* buf);

void oefs_bufu32_clear(oefs_bufu32_t* buf);

int oefs_bufu32_resize(oefs_bufu32_t* buf, uint32_t new_size);

int oefs_bufu32_append(oefs_bufu32_t* buf, const uint32_t* data, uint32_t size);

OE_EXTERNC_END

#endif /* _OEFS_BUF_H */
