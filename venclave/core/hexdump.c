// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "hexdump.h"
#include "lock.h"
#include "print.h"

static char _hexchar(uint8_t x)
{
    return (x <= 9) ? (char)(x + '0') : (char)(x - 0xa + 'a');
}

void ve_hexdump(const void* data, size_t size)
{
    const uint8_t* p = (const uint8_t*)data;
    size_t i;

    ve_lock(&__ve_print_lock);

    for (i = 0; i < size; i++)
    {
        char c1 = _hexchar((p[i] & 0xf0) >> 4);
        char c2 = _hexchar(p[i] & 0x0f);

        if (c1 == '0' && c2 == '0')
        {
            c1 = '.';
            c2 = '.';
        }

        ve_putc(c1);
        ve_putc(c2);

        if (((i + 1) % 16) == 0)
            ve_putc('\n');
        else if (i + 1 != size)
            ve_putc(' ');
    }

    ve_putc('\n');

    ve_unlock(&__ve_print_lock);
}
