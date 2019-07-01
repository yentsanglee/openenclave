// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/sgxtypes.h>

oe_result_t oe_ereport(
    sgx_target_info_t* target_info_align_512,
    sgx_report_data_t* report_data_align_128,
    sgx_report_t* report_align_512)
{
    /* Invoke EREPORT instruction */
    asm volatile("ENCLU"
                 :
                 : "a"(ENCLU_EREPORT),
                   "b"(target_info_align_512),
                   "c"(report_data_align_128),
                   "d"(report_align_512)
                 : "memory");

    return OE_OK;
}
