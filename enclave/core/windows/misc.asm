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
;; oe_exception_dispatcher()
;;
;; TODO: implement.
;;
;;=============================================================================
LEAF_ENTRY oe_exception_dispatcher, _TEXT$00
    ret
LEAF_END oe_exception_dispatcher, _TEXT$00

END
