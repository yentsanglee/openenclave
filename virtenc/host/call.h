// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_CALL_H
#define _VE_HOST_CALL_H

#include <openenclave/bits/types.h>
#include "../common/call.h"

void ve_handle_call_ping(uint64_t arg_in, uint64_t* arg_out);

#endif /* _VE_HOST_CALL_H */
