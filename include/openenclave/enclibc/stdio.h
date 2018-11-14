// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_STDIO_H
#define _ENCLIBC_STDIO_H

#include "bits/common.h"
#include "stdarg.h"

ENCLIBC_EXTERNC_BEGIN

typedef struct _ENCLIBC_IO_FILE ENCLIBC_FILE;
extern ENCLIBC_FILE* const enclibc_stdin;
extern ENCLIBC_FILE* const enclibc_stdout;
extern ENCLIBC_FILE* const enclibc_stderr;

int enclibc_vsnprintf(char* str, size_t size, const char* format, enclibc_va_list ap);

ENCLIBC_PRINTF_FORMAT(3, 4)
int enclibc_snprintf(char* str, size_t size, const char* format, ...);

int enclibc_vprintf(const char* format, enclibc_va_list ap);

ENCLIBC_PRINTF_FORMAT(1, 2)
int enclibc_printf(const char* format, ...);

#if defined(ENCLIBC_NEED_STDC_NAMES)

typedef ENCLIBC_FILE FILE;
#define stdin enclibc_stdin
#define stdout enclibc_stdout
#define stderr enclibc_stderr

ENCLIBC_INLINE
int vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    return enclibc_vsnprintf(str, size, format, ap);
}

ENCLIBC_PRINTF_FORMAT(3, 4)
ENCLIBC_INLINE
int snprintf(char* str, size_t size, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    return enclibc_vsnprintf(str, size, format, ap);
    va_end(ap);
}

ENCLIBC_INLINE
int vprintf(const char* format, va_list ap)
{
    return enclibc_vprintf(format, ap);
}

ENCLIBC_PRINTF_FORMAT(1, 2)
ENCLIBC_INLINE
int printf(const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    return enclibc_vprintf(format, ap);
    va_end(ap);
}

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

ENCLIBC_EXTERNC_END

#endif /* _ENCLIBC_STDIO_H */
