// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_TRACE_H
#define _VE_TRACE_H

void __ve_trace(const char* file, unsigned int line);

#define VE_TRACE __ve_trace(__FILE__, __LINE__)

#endif /* _VE_TRACE_H */
