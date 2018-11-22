// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_OEFS_RAISE_H
#define _OE_OEFS_RAISE_H

// clang-format off
#define CHECK(ERR)                     \
    do                                 \
    {                                  \
        int __err__ = ERR; \
        if (__err__ != 0)         \
        {                              \
            err = __err__;             \
            goto done;                 \
        }                              \
    }                                  \
    while (0)
// clang-format on

// clang-format off
#define RAISE(ERR) \
    do             \
    {              \
        err = ERR; \
        goto done; \
    }              \
    while (0)
// clang-format on

#endif /* _OE_OEFS_RAISE_H */
