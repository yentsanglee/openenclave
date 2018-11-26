// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OEFS_RAISE_H
#define _OEFS_RAISE_H

// clang-format off
#define OEFS_CHECK(ERR)    \
    do                     \
    {                      \
        int __err__ = ERR; \
        if (__err__ != 0)  \
        {                  \
            err = __err__; \
            goto done;     \
        }                  \
    }                      \
    while (0)
// clang-format on

// clang-format off
#define OEFS_RAISE(ERR) \
    do                  \
    {                   \
        err = ERR;      \
        goto done;      \
    }                   \
    while (0)
// clang-format on

#endif /* _OEFS_RAISE_H */
