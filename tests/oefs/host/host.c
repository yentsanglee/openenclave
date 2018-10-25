// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "oefs_u.h"

int main(int argc, const char* argv[])
{
    oe_result_t r;
    oe_enclave_t* enclave;
    oe_enclave_type_t type = OE_ENCLAVE_TYPE_SGX;
    const uint32_t flags = oe_get_create_flags();
    int ret;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH EXT2_FILENAME\n", argv[0]);
        return 1;
    }

    r = oe_create_oefs_enclave(argv[1], type, flags, NULL, 0, &enclave);
    if (r != OE_OK)
    {
        fprintf(stderr, "%s: oe_create_oefs_enclave() failed\n", argv[0]);
        exit(1);
    }

    r = test_oefs(enclave, &ret, argv[2]);
    if (r != OE_OK || ret != 0)
    {
        fprintf(stderr, "%s: test_oefs() failed\n", argv[0]);
        exit(1);
    }

#if 1
    r = oe_terminate_enclave(enclave);
    if (r != OE_OK)
    {
        fprintf(stderr, "%s: oe_terminate_enclave() failed\n", argv[0]);
        exit(1);
    }
#endif

    printf("=== passed all tests (oefs)\n");

    return 0;
}
