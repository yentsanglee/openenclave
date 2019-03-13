// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/error.h>
#include <openenclave/internal/files.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include "exec_u.h"

void oe_fs_install_sgxfs(void);

int main(int argc, const char* argv[])
{
    oe_result_t r;
    oe_enclave_t* enclave = NULL;
    const uint32_t flags = oe_get_create_flags();
    const oe_enclave_type_t type = OE_ENCLAVE_TYPE_SGX;
    void* data;
    size_t size;
    const char* enclave_path;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s PROGRAM\n", argv[0]);
        return 1;
    }

    if (!(enclave_path = getenv("OEEXEC_ENCLAVE_PATH")))
    {
        fprintf(stderr, "%s please define OEEXEC_ENCLAVE_PATH\n", argv[0]);
        return 1;
    }

    oe_fs_install_sgxfs();

    r = __oe_load_file(argv[1], 0, &data, &size);
    OE_TEST(r == OE_OK);

    r = oe_create_exec_enclave(enclave_path, type, flags, NULL, 0, &enclave);
    OE_TEST(r == OE_OK);

    int ret;
    r = exec(enclave, &ret, data, size, (size_t)(argc - 1), argv + 1);
    OE_TEST(r == OE_OK);

    // printf("host: exit status=%d\n", ret);

    r = oe_terminate_enclave(enclave);
    OE_TEST(r == OE_OK);

    free(data);

    return ret;
}
