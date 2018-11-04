// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../edltestutils.h"

#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <algorithm>
#include <chrono>
#include "all_u.h"

#define ITERATIONS size_t(1500000)
void test_switchless_edl_ecalls(oe_enclave_t* enclave)
{
    printf("=== test_switchless_edl_ecalls starting\n");
    // Turn on switchless calling.
    oe_configure_switchless_calling(enclave, 1, 0);

    int a = 5;
    int b = 6;
    int c = 0;

    double switchless_elapsed = 0.0;
    double normal_elapsed = 0.0;
    {
        printf("Executing add_switchless %ld times.\n", ITERATIONS);
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            OE_TEST(add_switchless(enclave, &c, a, b) == OE_OK);
            OE_TEST(c == a + b);
            a++;
            b++;
        }
        auto end = std::chrono::high_resolution_clock::now();
        switchless_elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count() /
            1000.0;
        printf("Switchless calling took : %.2f seconds\n", switchless_elapsed);
    }

    {
        printf("Executing add_normal %ld times.\n", ITERATIONS);
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < ITERATIONS; ++i)
        {
            OE_TEST(add_normal(enclave, &c, a, b) == OE_OK);
            OE_TEST(c == a + b);
            a++;
            b++;
        }
        auto end = std::chrono::high_resolution_clock::now();
        normal_elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count() /
            1000.0;
        printf("Normall calling took : %.2f seconds\n", normal_elapsed);
    }

    printf(
        "Switchless calling speedup factor = %.2f\n",
        normal_elapsed / switchless_elapsed);

    printf("=== test_switchless_edl_ecalls passed\n");
}
