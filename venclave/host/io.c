// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "io.h"
#include <unistd.h>

#define ve_read read
#define ve_write write

#include "../common/io.c"
