// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/utils.h>

/*******************************************************************************
 * gs_report.c - Defines __report_securityfailure and __report_rangecheckfailure
*  for X64 (borrowed from VS CRT)
 *******************************************************************************/
void __cdecl __report_rangecheckfailure(void)
{
    oe_abort();
}

void __cdecl __report_securityfailure(unsigned long code)
{
    oe_abort();
}

void __cdecl __report_gsfailure(uint64_t StackCookie)
{
    oe_abort();
}