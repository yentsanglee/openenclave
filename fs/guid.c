// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "guid.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static const char _hex_char[] =
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

FS_INLINE size_t _format_hex_byte(char buf[3], uint8_t x)
{
    buf[0] = _hex_char[(0xF0 & x) >> 4];
    buf[1] = _hex_char[0x0F & x];
    buf[2] = '\0';
    return 2;
}

void fs_format_guid(char buf[FS_GUID_STRING_SIZE], const fs_guid_t* guid)
{
    size_t n = 0;
    size_t m = 0;
    size_t i;

    n += _format_hex_byte(&buf[n], (guid->data1 & 0xFF000000) >> 24);
    n += _format_hex_byte(&buf[n], (guid->data1 & 0x00FF0000) >> 16);
    n += _format_hex_byte(&buf[n], (guid->data1 & 0x0000FF00) >> 8);
    n += _format_hex_byte(&buf[n], guid->data1 & 0x000000FF);
    buf[n++] = '-';

    n += _format_hex_byte(&buf[n], (guid->data2 & 0xFF00) >> 8);
    n += _format_hex_byte(&buf[n], guid->data2 & 0x00FF);
    buf[n++] = '-';

    n += _format_hex_byte(&buf[n], (guid->data3 & 0xFF00) >> 8);
    n += _format_hex_byte(&buf[n], guid->data3 & 0x00FF);
    buf[n++] = '-';

    n += _format_hex_byte(&buf[n], guid->data4[m++]);
    n += _format_hex_byte(&buf[n], guid->data4[m++]);
    buf[n++] = '-';

    n += _format_hex_byte(&buf[n], guid->data4[m++]);
    n += _format_hex_byte(&buf[n], guid->data4[m++]);
    n += _format_hex_byte(&buf[n], guid->data4[m++]);
    n += _format_hex_byte(&buf[n], guid->data4[m++]);
    n += _format_hex_byte(&buf[n], guid->data4[m++]);
    n += _format_hex_byte(&buf[n], guid->data4[m++]);
    buf[n++] = '\0';

    for (i = 0; i < n; i++)
        buf[i] = tolower(buf[i]);
}
