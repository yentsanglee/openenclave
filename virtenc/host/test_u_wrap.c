// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <string.h>
#include "hostmalloc.h"

#define malloc ve_host_malloc
#define free ve_host_free

#include "test_u.c"
