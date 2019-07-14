// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_PROXY_TRACE_H
#define _VE_PROXY_TRACE_H

#include <stdio.h>

#define VE_TRACE                                              \
    do                                                        \
    {                                                         \
        printf("proxy: trace: %s(%u)\n", __FILE__, __LINE__); \
        fflush(stdout);                                       \
    } while (0)

#endif /* _VE_PROXY_TRACE_H */
