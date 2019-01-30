// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENDIAN_H
#define _ENDIAN_H 1

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __PDP_ENDIAN 3412

#if defined(__i386) || defined(__x86_64)
#define __BYTE_ORDER __LITTLE_ENDIAN
#elif defined(__arm__) || defined(__aarch64__)
#define __BYTE_ORDER __BIG_ENDIAN
#endif

#endif
