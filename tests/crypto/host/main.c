// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <openenclave/internal/crypto/cert.h>
#include <openenclave/internal/crypto/ec.h>
#include <openenclave/internal/crypto/random.h>
#include <openenclave/internal/crypto/rsa.h>
#include <openenclave/internal/crypto/sha.h>
#include <openenclave/internal/hexdump.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../tests.h"

const char* arg0;

int main(int argc, const char* argv[])
{
    OE_UNUSED(argc);
    arg0 = argv[0];

    /* Run the tests */
    TestAll();

    printf("=== passed all tests (%s)\n", arg0);

    return 0;
}
