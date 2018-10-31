// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _fs_trace_h
#define _fs_trace_h

#include <stdio.h>

#define TRACE printf("%s(%u): %s()\n", __FILE__, __LINE__, __FUNCTION__)

#endif /* _fs_trace_h */
