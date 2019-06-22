// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_WAITPID_H
#define _VE_WAITPID_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

int ve_waitpid(int pid, int* status, int options);

#endif /* _VE_WAITPID_H */
