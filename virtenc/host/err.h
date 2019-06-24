// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_ERR_H
#define _VE_HOST_ERR_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

OE_PRINTF_FORMAT(1, 2)
void err(const char* fmt, ...);

OE_PRINTF_FORMAT(1, 2)
void puterr(const char* fmt, ...);

#endif /* _VE_HOST_ERR_H */
