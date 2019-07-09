// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/sgxtypes.h>
#include <stdio.h>
#include <string.h>
#include "proxy_t.h"

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

    /* Execute the EGETKEY hardware instruction. */
    {
        extern uint64_t oe_egetkey(const sgx_key_request_t*, sgx_key_t*);
        OE_ALIGNED(SGX_KEY_REQUEST_ALIGNMENT) sgx_key_request_t request;
        OE_ALIGNED(SGX_KEY_ALIGNMENT) sgx_key_t key;

        memcpy(&request, sgx_key_request, sizeof(request));
        memset(&key, 0, sizeof(key));

        ret = oe_egetkey(&request, &key);

        if (ret == SGX_EGETKEY_SUCCESS)
            memcpy(sgx_key, &key, sgx_key_size);
    }

done:
    return ret;
}

uint64_t ereport_ecall(
    const void* target_info,
    size_t target_info_size,
    const void* report_data,
    size_t report_data_size,
    void* report,
    size_t report_size)
{
    uint64_t ret = (uint64_t)-1;

    if (!target_info || target_info_size != sizeof(sgx_target_info_t))
        goto done;

    if (!report_data || report_data_size != sizeof(sgx_report_data_t))
        goto done;

    if (!report || report_size != sizeof(sgx_report_t))
        goto done;

    /* Execute the EREPORT hardware instruction. */
    {
        extern oe_result_t oe_ereport(
            sgx_target_info_t*, sgx_report_data_t*, sgx_report_t*);
        OE_ALIGNED(512) sgx_target_info_t ti;
        OE_ALIGNED(128) sgx_report_data_t rd;
        OE_ALIGNED(512) sgx_report_t r;

        memcpy(&ti, target_info, sizeof(ti));
        memcpy(&rd, report_data, sizeof(rd));
        memset(&r, 0, sizeof(r));

        ret = oe_ereport(&ti, &rd, &r);

        if (ret == OE_OK)
            memcpy(report, &r, report_size);
    }

done:
    return ret;
}

/* ATTN: revisit whether more than one thread is needed. */
OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    8);   /* TCSCount */
