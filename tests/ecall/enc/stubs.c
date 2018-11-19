// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/edger8r/enclave.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/jump.h>
#include <openenclave/internal/tests.h>
#include "../args.h"

void __cdecl _RTC_Shutdown(void)
{
}

void __cdecl _RTC_InitBase(void)
{
}

void __fastcall _RTC_CheckStackVars(void* esp, void* fd)
{
}

void __cdecl _RTC_UninitUse(const char* varname)
{
}

void __fastcall __security_check_cookie(uint64_t cookie)
{
}

void memset(void* dst, const int c, size_t size)
{
    char* p = (char*)dst;
	for (int i = 0; i < size; i++)
	{
        p[i] = (char)c;
	}
}

void memcpy(void* dst, const void *src, size_t size)
{
    char* p = (char*)dst;
    const char* q = (const char*)src;
    for (int i = 0; i < size; i++)
    {
        p[i] = q[i];
    }
}

void __cdecl __report_rangecheckfailure(void)
{
}

#pragma pack(push, 1)
/*  Structure padded under 32-bit x86, to get consistent
    execution between 32/64 targets.
*/
typedef struct _RTC_ALLOCA_NODE
{
    int guard1;
    struct _RTC_ALLOCA_NODE* next;
    int dummypad;
    size_t allocaSize;
    int dummypad2;
    int guard2[3];
} _RTC_ALLOCA_NODE;
#pragma pack(pop)

void __fastcall _RTC_AllocaHelper(
    _RTC_ALLOCA_NODE* pAllocaBase,
    size_t cbSize,
    _RTC_ALLOCA_NODE** pAllocaInfoList)
{
    // validate arguments. let _RTC_CheckStackVars2 AV
    // later on.
    if (!pAllocaBase || !cbSize || !pAllocaInfoList)
        return;

        // fill _alloca with 0xcc
    {
        size_t i;
        uint8_t* p;

        for (i = 0, p = (uint8_t*)pAllocaBase; i < cbSize; i++)
        {
            *p++ = 0xcc;
        }
    }

    // update embedded _alloca checking data
    pAllocaBase->next = *pAllocaInfoList;
    pAllocaBase->allocaSize = cbSize;

    // update function _alloca list, to loop through at function
    // exit and check.
    *pAllocaInfoList = pAllocaBase;
}

typedef struct _RTC_vardesc
{
    int addr;
    int size;
    char* name;
} _RTC_vardesc;

typedef struct _RTC_framedesc
{
    int varCount;
    _RTC_vardesc* variables;
} _RTC_framedesc;

void __fastcall _RTC_CheckStackVars2(
    void* frame,
    _RTC_framedesc* v,
    _RTC_ALLOCA_NODE* allocaList)
{
}