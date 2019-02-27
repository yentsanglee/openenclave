// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_LIBC_CHECK_FIELD_H
#define _OE_LIBC_CHECK_FIELD_H

#include <openenclave/internal/defs.h>

#define CHECK_FIELD(T1, T2, F)                                  \
    OE_STATIC_ASSERT(OE_OFFSETOF(T1, F) == OE_OFFSETOF(T2, F)); \
    OE_STATIC_ASSERT(sizeof(((T1*)0)->F) == sizeof(((T2*)0)->F));

#endif /* _OE_LIBC_CHECK_FIELD_H */
