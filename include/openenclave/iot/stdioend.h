/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _OE_IOT_STDIOEND_H
#define _OE_IOT_STDIOEND_H

#if defined(_STDIOBEGIN_DEFINED_OE_ENCLAVE_H)
#undef _OE_ENCLAVE_H
#endif

#undef oe_fclose
#undef oe_feof
#undef oe_ferror
#undef oe_fflush
#undef oe_fgets
#undef oe_fprintf
#undef oe_fputs
#undef oe_fread
#undef oe_fseek
#undef oe_ftell
#undef oe_fvfprintf
#undef oe_fwrite
#undef oe_fopen_OE_FILE_INSECURE
#undef oe_fopen_OE_OE_FILE_SECURE_HARDWARE
#undef oe_fopen_OE_FILE_SECURE_ENCRYPTION
#undef oe_remove_OE_FILE_INSECURE
#undef oe_remove_OE_FILE_SECURE_HARDWARE
#undef oe_remove_OE_FILE_SECURE_ENCRYPTION
#undef oe_opendir_FILE_INSECURE
#undef oe_opendir_SECURE_HARDWARE
#undef oe_opendir_SECURE_ENCRYPTION

#endif /* _OE_IOT_STDIOEND_H */
