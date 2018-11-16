// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/utils.h>

/*******************************************************************************
 * gshandler.c - Defines __GSHandlerCheck for X64 (borrowed from VS CRT)
 *******************************************************************************/

/*
 * Exception disposition return values.
 */
typedef enum _EXCEPTION_DISPOSITION
{
    ExceptionContinueExecution,
    ExceptionContinueSearch,
    ExceptionNestedException,
    ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;

typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY
{
    uint32_t BeginAddress;
    uint32_t EndAddress;
    union {
        uint32_t UnwindInfoAddress;
        uint32_t UnwindData;
    };
} RUNTIME_FUNCTION;

typedef struct _DISPATCHER_CONTEXT
{
    uint64_t ControlPc;
    uint64_t ImageBase;
    RUNTIME_FUNCTION* FunctionEntry;
    uint64_t EstablisherFrame;
    uint64_t TargetIp;
    void* ContextRecord_unused;
    void* LanguageHandler_unused;
    void* HandlerData;
    void* HistoryTable_unused;
    uint32_t ScopeIndex_unused;
    uint32_t Fill0;
} DISPATCHER_CONTEXT;

//
// Define unwind information structure.
//
typedef union _UNWIND_CODE {
    struct
    {
        uint8_t CodeOffset;
        uint8_t UnwindOp : 4;
        uint8_t OpInfo : 4;
    };

    struct
    {
        uint8_t OffsetLow;
        uint8_t UnwindOp : 4;
        uint8_t OffsetHigh : 4;
    } EpilogueCode;

    uint16_t FrameOffset;
} UNWIND_CODE;

typedef struct _UNWIND_INFO
{
    uint8_t Version : 3;
    uint8_t Flags : 5;
    uint8_t SizeOfProlog;
    uint8_t CountOfCodes;
    uint8_t FrameRegister : 4;
    uint8_t FrameOffset : 4;
    UNWIND_CODE UnwindCode[1];

    //
    // The unwind codes are followed by an optional DWORD aligned field that
    // contains the exception handler address or a function table entry if
    // chained unwind information is specified. If an exception handler address
    // is specified, then it is followed by the language specified exception
    // handler data.
    //
    //  union {
    //      struct {
    //          ULONG ExceptionHandler;
    //          ULONG ExceptionData[];
    //      } DUMMYUNIONNAME;
    //
    //      RUNTIME_FUNCTION FunctionEntry;
    //  };
    //

} UNWIND_INFO, *PUNWIND_INFO;

typedef struct _GS_HANDLER_DATA
{
    union
    {
        struct
        {
            uint32_t EHandler : 1;
            uint32_t UHandler : 1;
            uint32_t HasAlignment : 1;
        } Bits;
        int32_t        CookieOffset;
    } u;
    int32_t AlignedBaseOffset;
    int32_t Alignment;
} GS_HANDLER_DATA;

void
__GSHandlerCheckCommon (
    const void* EstablisherFrame,
    const DISPATCHER_CONTEXT* DispatcherContext,
    const GS_HANDLER_DATA* GSHandlerData
    );

void __fastcall __security_check_cookie(uint64_t cookie);

    /***
*__GSHandlerCheck - Check local security cookie during exception handling
*                   on X64
*
*Purpose:
*   Functions which have a local security cookie, but do not use any exception
*   handling (either C++ EH or SEH), will register this routine for exception
*   handling.  It exists simply to check the validity of the local security
*   cookie during exception dispatching and unwinding.  This helps defeat
*   buffer overrun attacks that trigger an exception to avoid the normal
*   end-of-function cookie check.
*
*   Note that this routine must be statically linked into any module that uses
*   it, since it needs access to the global security cookie of that function's
*   image.
*
*Entry:
*   ExceptionRecord - Supplies a pointer to an exception record.
*   EstablisherFrame - Supplies a pointer to frame of the establisher function.
*   ContextRecord - Supplies a pointer to a context record.
*   DispatcherContext - Supplies a pointer to the exception dispatcher or
*       unwind dispatcher context.
*
*Return:
*   If the security cookie check fails, the process is terminated.  Otherwise,
*   return an exception disposition of continue execution.
*
*******************************************************************************/

EXCEPTION_DISPOSITION
__GSHandlerCheck (
    const void* ExceptionRecord_unused,
    const void* EstablisherFrame,
    void* ContextRecord_unused,
    DISPATCHER_CONTEXT* DispatcherContext
    )
{
    //
    // The common helper performs all the real work.
    //

    __GSHandlerCheckCommon(EstablisherFrame,
                           DispatcherContext,
                           (GS_HANDLER_DATA*)DispatcherContext->HandlerData);

    //
    // The cookie check succeeded, so continue the search for exception or
    // termination handlers.
    //

    return ExceptionContinueSearch;
}

/***
*__GSHandlerCheckCommon - Helper for exception handlers checking the local
*                         security cookie during exception handling on X64
*
*Purpose:
*   This performs the actual local security cookie check for the three
*   exception handlers __GSHandlerCheck, __GSHandlerCheck_SEH, and
*   __GSHandlerCheckEH.
*
*   Note that this routine must be statically linked into any module that uses
*   it, since it needs access to the global security cookie of that function's
*   image.
*
*Entry:
*   EstablisherFrame - Supplies a pointer to frame of the establisher function.
*   DispatcherContext - Supplies a pointer to the exception dispatcher or
*       unwind dispatcher context.
*   GSHandlerData - Supplies a pointer to the portion of the language-specific
*       handler data that is used to find the local security cookie in the
*       frame.
*
*Return:
*   If the security cookie check fails, the process is terminated.
*
*******************************************************************************/

void
__GSHandlerCheckCommon (
    const void* EstablisherFrame,
    const DISPATCHER_CONTEXT* DispatcherContext,
    const GS_HANDLER_DATA* GSHandlerData)
{
    int32_t CookieOffset;
    char* CookieFrameBase;
    uint64_t Cookie;
    UNWIND_INFO* UnwindInfo;
    uint64_t CookieXorValue;

    //
    // Find the offset of the local security cookie within the establisher
    // function's call frame, from some as-yet undetermined base point.  The
    // bottom 3 bits of the cookie offset DWORD are used for other purposes,
    // so zero them out.
    //

    CookieOffset = GSHandlerData->u.CookieOffset & ~0x07;

    //
    // The base from which the cookie offset is applied is either the RSP
    // within that frame, passed here as EstablisherFrame, or in the case of
    // dynamically-aligned frames, an aligned address offset from RSP.
    //

    CookieFrameBase = (char*)EstablisherFrame;
    if (GSHandlerData->u.Bits.HasAlignment)
    {
        CookieFrameBase =
            (char*)
             (((uint64_t)EstablisherFrame + GSHandlerData->AlignedBaseOffset)
              & -GSHandlerData->Alignment);
    }

    //
    // Retrieve the local security cookie, now that we have determined its
    // location.
    //

    Cookie = *(uint64_t*)(CookieFrameBase + CookieOffset);

    //
    // The local cookie is XORed with the frame pointer, which may be offset
    // from the EstablisherFrame the OS passed us.  Check the unwind info to
    // see if an offset frame pointer was used.
    //

    UnwindInfo = (UNWIND_INFO*)(DispatcherContext->FunctionEntry->UnwindData
                                + DispatcherContext->ImageBase);
    CookieXorValue = (uint64_t)EstablisherFrame;
    if (UnwindInfo->FrameRegister != 0)
    {
        CookieXorValue += UnwindInfo->FrameOffset * 16;
    }

    //
    // Rematerialize the original cookie, and if it doesn't match the global
    // cookie, or fails any additional checks __security_check_cookie may
    // perform, issue a fatal Watson dump.
    //

    Cookie ^= CookieXorValue;
    __security_check_cookie(Cookie);
}
