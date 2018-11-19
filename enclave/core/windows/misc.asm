;; Copyright (c) Microsoft Corporation. All rights reserved.
;; Licensed under the MIT License.

include ksamd64.inc

ENCLU_EREPORT                   EQU 0
ENCLU_EGETKEY					EQU 1
;;=============================================================================
;;
;; oe_rand()
;;
;;=============================================================================
LEAF_ENTRY oe_rand, _TEXT$00
    rdrand  rax
    ret
LEAF_END oe_rand, _TEXT$00

;;==============================================================================
;;
;; void oe_issue_sgx_ereport(
;;    sgx_target_info_t* ti,
;;    sgx_report_data_t* rd,
;;    sgx_report_t* r);
;;
;; Registers:
;;     RCX - ti
;;     RDX - ri
;;     R8 - r
;;
;; Purpose:
;;     Issue EREPROT instruction.
;;
;;==============================================================================

NESTED_ENTRY oe_issue_sgx_ereport, _TEXT$00
    ;; Setup stack frame:
    rex_push_reg rbx
    END_PROLOGUE

    ;; EREPROT(RAX=EREPROT, RBX=ti, RCX=rd, RDX=r)
    mov     eax, ENCLU_EREPORT
    mov     rbx, rcx
    mov     rcx, rdx
    mov     rdx, r8
    ENCLU

    BEGIN_EPILOGUE
    pop     rbx
    ret

NESTED_END oe_issue_sgx_ereport, _TEXT$00

;;==============================================================================
;;
;; uint64_t oe_egetkey(const SgxKeyRequest* sgx_key_request, SgxKey* sgx_key);
;;
;;     The EGETKEY instruction wrapper.
;;
;;     Registers:
;;         RCX - sgx_key_request
;;         RDX - sgx_key
;;
;;     return:
;;         Return values in RAX
;;             SGX_EGETKEY_SUCCESS
;;             SGX_EGETKEY_INVALID_ATTRIBUTE
;;             SGX_EGETKEY_INVALID_CPUSVN
;;             SGX_EGETKEY_INVALID_ISVSVN
;;             SGX_EGETKEY_INVALID_KEYNAME
;;==============================================================================
NESTED_ENTRY oe_egetkey, _TEXT$00
    ;; Setup stack frame:
    rex_push_reg rbx
    END_PROLOGUE

    ;; EREPROT(RAX=ENCLU_EGETKEY, RBX=sgx_key_request, RCX=sgx_key)
    mov     eax, ENCLU_EGETKEY
    mov     rbx, rcx
    mov     rcx, rdx
    ENCLU

    BEGIN_EPILOGUE
    pop     rbx
    ret

NESTED_END oe_egetkey, _TEXT$00

;;=============================================================================
;;
;; Contxt fields - must match exactly the lay of oe_context_t
;;
;;=============================================================================
;;
ctx_t struct
	flags		dq ?
	_rax		dq ?
	_rbx		dq ?
	_rcx		dq ?
	_rdx		dq ?
	_rbp		dq ?
	_rsp		dq ?
	_rdi		dq ?
	_rsi		dq ?
	_r8 		dq ?
	_r9 		dq ?
	_r10		dq ?
	_r11		dq ?
	_r12		dq ?
	_r13		dq ?
	_r14		dq ?
	_r15		dq ?
	_rip		dq ?
	mxcsr		dd ?
	filler		dd 3 dup (?)
	_xstate		db 512 dup (?)
ctx_t ends

;;==============================================================================
;;
;; This macro is used to restore the X87, SSE, rax, rbx, rdx, rdi, rsi, and r8,
;; r9 ... r15 from saved oe_context_t.
;;   (rcx) - ptr to oe_context_t
;;
;;==============================================================================

restore_common_regsters macro
	fxrstor		[rcx].ctx_t._xstate
	ldmxcsr		[rcx].ctx_t.mxcsr
	mov			rax, [rcx].ctx_t._rax
	mov			rbx, [rcx].ctx_t._rbx
	mov			rdx, [rcx].ctx_t._rdx
	mov			rdi, [rcx].ctx_t._rdi
	mov			rsi, [rcx].ctx_t._rsi
	mov			r8,  [rcx].ctx_t._r8
	mov			r9,  [rcx].ctx_t._r9
	mov			r10, [rcx].ctx_t._r10
	mov			r11, [rcx].ctx_t._r11
	mov			r12, [rcx].ctx_t._r12
	mov			r13, [rcx].ctx_t._r13
	mov			r14, [rcx].ctx_t._r14
	mov			r15, [rcx].ctx_t._r15
	endm	;; restore_common_regsters

;;==============================================================================
;;
;; void oe_continue_execution(oe_context_t* oe_context)
;;
;; Routine Description:
;;
;;   This function restores the full oe_context, and continue run on the rip of
;;   input context.
;;
;; N.B. Prolog/Epilog for this function is likely not implemented correctly. Need to
;;      get it to work or exception unwinding can choke.
;;
;; Arguments:
;;
;;    oe_context (cx) - Supplies a pointer to a context record.
;;
;; Return Value:
;;
;;    None. This function will not return to caller.
;;
;;==============================================================================
NESTED_ENTRY oe_continue_execution, _TEXT$00
    ;; Setup stack frame:
    rex_push_reg rbp
    set_frame    rbp, 0
    END_PROLOGUE

	;; restore common registers
	restore_common_regsters

	;; restore rsp
	mov		rsp, [rcx].ctx_t._rsp

	;; setup stack
	push	qword ptr [rcx].ctx_t._rip
	push	qword ptr [rcx].ctx_t._rbp
	push	qword ptr [rcx].ctx_t.flags

	;; restore rcx
	mov		rcx, [rcx].ctx_t._rcx

	;; reload flags
	popfq

	BEGIN_EPILOGUE
	pop		rbp
	ret

NESTED_END oe_continue_execution, _TEXT$00


;;=============================================================================
;;
;; oe_exception_dispatcher()
;;
;; TODO: implement.
;;
;;=============================================================================
LEAF_ENTRY oe_exception_dispatcher, _TEXT$00
    ret
LEAF_END oe_exception_dispatcher, _TEXT$00

END
