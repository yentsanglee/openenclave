// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "string.h"

size_t ve_strlen(const char* s)
{
    const char* p = s;

    while (*p)
        p++;

    return p - s;
}

int ve_strcmp(const char* s1, const char* s2)
{
    while ((*s1 && *s2) && (*s1 == *s2))
    {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

size_t ve_strlcpy(char* dest, const char* src, size_t size)
{
    const char* start = src;

    if (size)
    {
        char* end = dest + size - 1;

        while (*src && dest != end)
            *dest++ = (char)*src++;

        *dest = '\0';
    }

    while (*src)
        src++;

    return (size_t)(src - start);
}

size_t ve_strlcat(char* dest, const char* src, size_t size)
{
    size_t n = 0;

    if (size)
    {
        char* end = dest + size - 1;

        while (*dest && dest != end)
        {
            dest++;
            n++;
        }

        while (*src && dest != end)
        {
            n++;
            *dest++ = *src++;
        }

        *dest = '\0';
    }

    while (*src)
    {
        src++;
        n++;
    }

    return n;
}

static __inline__ char oe_get_hex_char(uint64_t x, size_t i)
{
    uint64_t nbits = (uint64_t)i * 4;
    char nibble = (char)((x & (0x000000000000000fUL << nbits)) >> nbits);
    return (char)((nibble < 10) ? ('0' + nibble) : ('a' + (nibble - 10)));
}

const char* ve_uint64_octstr(ve_intstr_buf_t* buf, uint64_t x, size_t* size)
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

const char* ve_uint64_decstr(ve_intstr_buf_t* buf, uint64_t x, size_t* size)
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

const char* ve_uint64_hexstr(ve_intstr_buf_t* buf, uint64_t x, size_t* size)
{
    for (size_t i = 0; i < 16; i++)
        buf->data[15 - i] = oe_get_hex_char(x, i);

    buf->data[16] = '\0';

    const char* p = buf->data;

    /* Skip over leading zeros (but not the final zero) */
    while (p[0] && p[1] && p[0] == '0')
        p++;

    if (size)
        *size = ve_strlen(p);

    return p;
}

const char* ve_int64_decstr(ve_intstr_buf_t* buf, int64_t x, size_t* size)
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

void* ve_memset(void* s, int c, size_t n)
{
    volatile uint8_t* p = (volatile uint8_t*)s;

    while (n--)
        *p++ = c;

    return s;
}

void* memset(void* s, int c, size_t n)
{
    volatile uint8_t* p = (volatile uint8_t*)s;

    while (n--)
        *p++ = c;

    return s;
}

void* ve_memcpy(void* dest, const void* src, size_t n)
{
    uint8_t* p = (uint8_t*)dest;
    const uint8_t* q = (uint8_t*)src;

    while (n--)
        *p++ = *q++;

    return dest;
}

int ve_memcmp(const void* s1, const void* s2, size_t n)
{
    const unsigned char* p = (const unsigned char*)s1;
    const unsigned char* q = (const unsigned char*)s2;

    while (n--)
    {
        int r = *p++ - *q++;

        if (r)
            return r;
    }

    return 0;
}
