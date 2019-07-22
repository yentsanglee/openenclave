// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "tests.h"

void TestAll()
{
    // TestASN1();
    // TestCRL();
    TestRSA(); // Test for regressions in keys.c
    TestEC();
    TestRandom();
    TestRdrand();
    TestHMAC();
    TestKDF();
    TestSHA();
}
