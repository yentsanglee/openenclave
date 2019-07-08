// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/sgxtypes.h>
#include <stdio.h>
#include <string.h>
#include "proxy_t.h"

void dummy_ecall(void)
{
    printf("oevproxyenc: dummy_ecall()\n");
    fflush(stdout);
}

uint64_t egetkey_ecall(
    const void* sgx_key_request,
    size_t sgx_key_request_size,
    void* sgx_key,
    size_t sgx_key_size)
{
    uint64_t ret = SGX_EGETKEY_INVALID_ATTRIBUTE;

    if (!sgx_key_request || sgx_key_request_size != sizeof(sgx_key_request_t))
        goto done;

    if (!sgx_key || sgx_key_size != sizeof(sgx_key_t))
        goto done;

    /* Execute EGETKEY hardware instruction. */
    {
        extern uint64_t oe_egetkey(const sgx_key_request_t*, sgx_key_t*);
        OE_ALIGNED(SGX_KEY_REQUEST_ALIGNMENT) sgx_key_request_t request;
        OE_ALIGNED(SGX_KEY_ALIGNMENT) sgx_key_t key;

        memcpy(&request, sgx_key_request, sizeof(request));
        memset(&key, 0, sizeof(key));

        ret = oe_egetkey(&request, &key);

        printf("RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR=%lu\n", ret);
        if (ret == SGX_EGETKEY_SUCCESS)
            memcpy(sgx_key, &key, sgx_key_size);
    }

done:
    return ret;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    8);   /* TCSCount */
