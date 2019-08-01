// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_FUNCNAME_H
#define _OE_FUNCNAME_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/result.h>

OE_EXTERNC_BEGIN

oe_result_t oe_get_func_name(void* addr, char* buf, size_t size);

OE_EXTERNC_END

#endif /* _OE_FUNCNAME_H */
