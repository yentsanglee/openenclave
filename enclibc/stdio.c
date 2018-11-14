// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/enclavelibc.h>
#include <stdio.h>

ENCLIBC_FILE* const enclibc_stdin = ((ENCLIBC_FILE*)0x1000000000000001);
ENCLIBC_FILE* const enclibc_stdout = ((ENCLIBC_FILE*)0x1000000000000002);
ENCLIBC_FILE* const enclibc_stderr = ((ENCLIBC_FILE*)0x1000000000000003);
