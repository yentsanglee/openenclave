// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/print.h>

int enclibc_vsnprintf(
    char* str, size_t size, const char* fmt, enclibc_va_list ap)
{
    return oe_vsnprintf(str, size, fmt, ap);
}

int enclibc_snprintf(char* str, size_t size, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(str, size, fmt, ap);
    va_end(ap);
    return n;
}

int enclibc_vprintf(const char* fmt, enclibc_va_list ap_)
{
    char buf[256];
    char* p = buf;
    char* new_buf = NULL;
    int n = -1;

    /* Try first with a fixed-length scratch buffer */
    {
        va_list ap;
        va_copy(ap, ap_);
        n = vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

        if (n < sizeof(buf))
        {
            oe_host_write(0, p, (size_t)-1);
            goto done;
        }
    }

    /* If string was truncated, retry with correctly sized buffer */
    {
        if (!(new_buf = malloc(n + 1)))
            goto done;

        p = new_buf;

        va_list ap;
        va_copy(ap, ap_);
        n = vsnprintf(p, n + 1, fmt, ap);
        va_end(ap);

        oe_host_write(0, p, (size_t)-1);
    }

done:

    if (new_buf)
        free(new_buf);

    return n;
}

int enclibc_printf(const char* format, ...)
{
    int n;
    va_list ap;

    va_start(ap, format);
    n = vprintf(format, ap);
    va_end(ap);

    return n;
}
