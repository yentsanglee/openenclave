// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "buf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
**==============================================================================
**
** oefs_buf_t:
**
**==============================================================================
*/

#define EXT2_BUF_CHUNK_SIZE 1024

void oefs_buf_release(oefs_buf_t* buf)
{
    if (buf && buf->data)
    {
        memset(buf->data, 0xDD, buf->size);
        free(buf->data);
    }

    memset(buf, 0x00, sizeof(oefs_buf_t));
}

int oefs_buf_clear(oefs_buf_t* buf)
{
    if (!buf)
        return -1;

    buf->size = 0;

    return 0;
}

int oefs_buf_reserve(oefs_buf_t* buf, uint32_t cap)
{
    if (!buf)
        return -1;

    /* If capacity is bigger than current capacity */
    if (cap > buf->cap)
    {
        void* new_data;
        uint32_t new_cap;

        /* Double current capacity (will be zero the first time) */
        new_cap = buf->cap * 2;

        /* If capacity still insufficent, round to multiple of chunk size */
        if (cap > new_cap)
        {
            const uint32_t N = EXT2_BUF_CHUNK_SIZE;
            new_cap = (cap + N - 1) / N * N;
        }

        /* Expand allocation */
        if (!(new_data = realloc(buf->data, new_cap)))
            return -1;

        buf->data = new_data;
        buf->cap = new_cap;
    }

    return 0;
}

int oefs_buf_resize(oefs_buf_t* buf, uint32_t new_size)
{
    uint8_t* data;

    if (!buf)
        return -1;

    if (new_size == 0)
    {
        oefs_buf_release(buf);
        memset(buf, 0, sizeof(oefs_buf_t));
        return 0;
    }

    if (!(data = realloc(buf->data, new_size)))
        return -1;

    if (new_size > buf->size)
        memset(data + buf->size, 0, new_size - buf->size);

    buf->data = data;
    buf->size = new_size;
    buf->cap = new_size;

    return 0;
}

int oefs_buf_append(oefs_buf_t* buf, const void* data, uint32_t size)
{
    uint32_t new_size;

    /* Check arguments */
    if (!buf || !data)
        return -1;

    /* If zero-sized, then success */
    if (size == 0)
        return 0;

    /* Compute the new size */
    new_size = buf->size + size;

    /* If insufficient capacity to hold new data */
    if (new_size > buf->cap)
    {
        int err;

        if ((err = oefs_buf_reserve(buf, new_size)) != 0)
            return err;
    }

    /* Copy the data */
    memcpy((unsigned char*)buf->data + buf->size, data, size);
    buf->size = new_size;

    return 0;
}

/*
**==============================================================================
**
** oefs_bufu32_t:
**
**==============================================================================
*/

void oefs_bufu32_release(oefs_bufu32_t* buf)
{
    oefs_buf_t tmp;

    tmp.data = buf->data;
    tmp.size = buf->size * sizeof(uint32_t);
    tmp.cap = buf->cap * sizeof(uint32_t);
    oefs_buf_release(&tmp);
}

void oefs_bufu32_clear(oefs_bufu32_t* buf)
{
    oefs_bufu32_release(buf);
    buf->data = NULL;
    buf->size = 0;
    buf->cap = 0;
}

int oefs_bufu32_append(oefs_bufu32_t* buf, const uint32_t* data, uint32_t size)
{
    oefs_buf_t tmp;

    tmp.data = buf->data;
    tmp.size = buf->size * sizeof(uint32_t);
    tmp.cap = buf->cap * sizeof(uint32_t);

    if (oefs_buf_append(&tmp, data, size * sizeof(uint32_t)) != 0)
    {
        return -1;
    }

    buf->data = tmp.data;
    buf->size = tmp.size / sizeof(uint32_t);
    buf->cap = tmp.cap / sizeof(uint32_t);

    return 0;
}

int oefs_bufu32_resize(oefs_bufu32_t* buf, uint32_t new_size)
{
    oefs_buf_t tmp;

    tmp.data = buf->data;
    tmp.size = buf->size * sizeof(uint32_t);
    tmp.cap = buf->cap * sizeof(uint32_t);

    if (oefs_buf_resize(&tmp, new_size * sizeof(uint32_t)) != 0)
        return -1;

    buf->data = tmp.data;
    buf->size = tmp.size / sizeof(uint32_t);
    buf->cap = tmp.cap / sizeof(uint32_t);

    return 0;
}
