// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "abort.h"
#include "exit.h"

void ve_abort()
{
    *((volatile int*)0) = 0;
    ve_exit(127);

    for (;;)
        ;
}
