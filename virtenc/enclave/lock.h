// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_LOCK_H
#define _VE_LOCK_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

typedef volatile uint32_t ve_lock_t;

int ve_lock(ve_lock_t* lock);

int ve_unlock(ve_lock_t* lock);

#endif /* _VE_LOCK_H */
