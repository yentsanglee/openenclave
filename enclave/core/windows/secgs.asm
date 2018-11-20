;; Copyright (c) Microsoft Corporation. All rights reserved.
;; Licensed under the MIT License.

include ksamd64.inc

        extern __security_cookie:QWORD
        extern __report_gsfailure:PROC

;;==============================================================================
;; Implementation of __security_check_cookie, borrowed from VS CRT
;;==============================================================================

;*++
;
; VOID
; __security_check_cookie (
;    ULONG64 Value
;    )
;
; Routine Description:
;
;   This function checks the specified cookie value against the global
;   cookie value. If the values match, then control is returned to the
;   caller. Otherwise, the failure is reported and there is no return
;   to the caller.
;
;   N.B. No registers except for RCX are modified if the return is to the
;   caller.
;
; Arguments:
;
;   Value (rcx) - Supplies the value of cookie.
;
; Return Value:
;
;    None.
;
;--*

        LEAF_ENTRY __security_check_cookie, _TEXT$00

        cmp rcx, __security_cookie      ; check cookie value in frame
    bnd jne ReportFailure               ; if ne, cookie check failure
        rol rcx, 16                     ; make sure high word is zero
        test cx, -1
    bnd jne RestoreRcx
    bnd ret                             ; use BND RET to preserve contents
                                        ; of MPX Bounds registers. Shouldn't use
                                        ; one byte return after conitional jump
                                        ; BND RET is 2 bytes, so it's OK

;
; The cookie check failed.
;

RestoreRcx:
        ror rcx, 16

ReportFailure:

        jmp __report_gsfailure          ; overrun found

        LEAF_END __security_check_cookie, _TEXT$00

END
