/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#include "string.h"

static char _get_hex_char(uint64_t x, size_t i)
{
    uint64_t nbits = (uint64_t)i * 4;
    char nibble = (char)((x & (0x000000000000000fUL << nbits)) >> nbits);
    return (char)((nibble < 10) ? ('0' + nibble) : ('a' + (nibble - 10)));
}

const char* ve_ostr(ve_ostr_buf* buf, uint64_t x, size_t* size)
{
    char* p;
    char* end = buf->data + sizeof(buf->data) - 1;

    p = end;
    *p = '\0';

    do
    {
        *--p = (char)('0' + x % 8);
    } while (x /= 8);

    if (size)
        *size = (size_t)(end - p);

    return p;
}

const char* ve_ustr(ve_ustr_buf* buf, uint64_t x, size_t* size)
{
    char* p;
    char* end = buf->data + sizeof(buf->data) - 1;

    p = end;
    *p = '\0';

    do
    {
        *--p = (char)('0' + x % 10);
    } while (x /= 10);

    if (size)
        *size = (size_t)(end - p);

    return p;
}

const char* ve_xstr(ve_xstr_buf* buf, uint64_t x, size_t* size)
{
    size_t i;

    for (i = 0; i < 16; i++)
        buf->data[15 - i] = _get_hex_char(x, i);

    buf->data[16] = '\0';

    const char* p = buf->data;

    /* Skip over leading zeros (but not the final zero) */
    while (p[0] && p[1] && p[0] == '0')
        p++;

    if (size)
        *size = oe_strlen(p);

    return p;
}

const char* ve_dstr(ve_dstr_buf* buf, int64_t x, size_t* size)
{
    char* p;
    int neg = 0;
    static const char _str[] = "-9223372036854775808";
    char* end = buf->data + sizeof(buf->data) - 1;

    if (x == OE_INT64_MIN)
    {
        *size = sizeof(_str) - 1;
        return _str;
    }

    if (x < 0)
    {
        neg = 1;
        x = -x;
    }

    p = end;
    *p = '\0';

    do
    {
        *--p = (char)('0' + x % 10);
    } while (x /= 10);

    if (neg)
        *--p = '-';

    if (size)
        *size = (size_t)(end - p);

    return p;
}
