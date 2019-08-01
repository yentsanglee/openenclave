// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ctype.h>
#include <execinfo.h>
#include <openenclave/internal/funcname.h>
#include <openenclave/internal/raise.h>
#include <stdlib.h>
#include <string.h>

oe_result_t oe_get_func_name(void* addr, char* buf, size_t size)
{
    oe_result_t result = OE_UNEXPECTED;
    void* buffer[1] = {(void*)addr};
    char** symbols = NULL;
    char* start;
    char* end;

    /* Check for invalid parameters. */
    if (!addr || !buf || !size)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Obtain the funciton name associated with this address. */
    if (!(symbols = backtrace_symbols(buffer, 1)) || !symbols[0])
        OE_RAISE(OE_FAILURE);

    /* Find the start of the function name.
     * Example: /usr/bin(myfunc+0) [0x41d360]
     */
    {
        /* Find the first opening parenthesis. */
        if (!(start = strchr(symbols[0], '(')))
            OE_RAISE(OE_FAILURE);

        /* Skip over the opening parenthesis. */
        start++;
    }

    /* Find the end of the function name. */
    {
        end = start;

        while (*end && (isalnum(*end) || *end == '_'))
            end++;

        *end = '\0';
    }

    /* Copy the function name to the caller's buffer. */
    *buf = '\0';
    strncat(buf, start, size - 1);

    result = OE_OK;

done:

    if (symbols)
        free(symbols);

    return result;
}
