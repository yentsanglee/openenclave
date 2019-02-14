// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/limits.h>
#include <openenclave/corelibc/pthread.h>

#if !defined(CHAR_BIT)
#define CHAR_BIT OE_CHAR_BIT
#endif

#if !defined(pthread_mutex_t)
#define pthread_mutex_t oe_pthread_mutex_t
#endif
