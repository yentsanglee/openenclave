// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/error.h>
#include <openenclave/internal/files.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include "exec_u.h"

int main(int argc, const char* argv[])
{
    oe_result_t r;
    oe_enclave_t* enclave = NULL;
    const uint32_t flags = oe_get_create_flags();
    const oe_enclave_type_t type = OE_ENCLAVE_TYPE_SGX;
    void* data;
    size_t size;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH PROGRAM\n", argv[0]);
        return 1;
    }

    r = __oe_load_file(argv[2], 0, &data, &size);
    OE_TEST(r == OE_OK);

    r = oe_create_exec_enclave(argv[1], type, flags, NULL, 0, &enclave);
    OE_TEST(r == OE_OK);

    r = test_exec(enclave, data, size);
    OE_TEST(r == OE_OK);

    r = oe_terminate_enclave(enclave);
    OE_TEST(r == OE_OK);

    free(data);

    return 0;
}
