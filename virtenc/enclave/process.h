// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_PROCESS_H
#define _VE_PROCESS_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

void ve_exit(int status);

__attribute__((__noreturn__)) void ve_abort(void);

int ve_gettid(void);

int ve_getpid(void);

int ve_waitpid(int pid, int* status, int options);

#endif /* _VE_PROCESS_H */
